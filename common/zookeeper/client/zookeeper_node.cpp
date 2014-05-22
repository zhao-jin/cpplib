// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <algorithm>
#include <set>
#include <sstream>
#include "common/base/scoped_ptr.h"
#include "common/base/string/algorithm.h"
#include "common/zookeeper/client/zookeeper_acl.h"
#include "common/zookeeper/client/zookeeper_node.h"
#include "common/zookeeper/client/zookeeper_session.h"

namespace common {

static const char kLockNodePrefix[] = "zoo_attr_lock_";

namespace { // anonymous namespace

void NodeWatcher(zhandle_t* handle, int type, int state, const char* path, void* context)
{
    ZookeeperSession* session = static_cast<ZookeeperSession*>(context);
    std::set<ZookeeperNode::WatcherCallback*> watchers;
    session->GetWatcherManager()->CollectWatchers(path, type, &watchers);
    std::set<ZookeeperNode::WatcherCallback*>::iterator iter;
    for (iter = watchers.begin(); iter != watchers.end(); ++iter) {
        (*iter)->Run(path, type);
    }
}

void ProcessNodeWatcher(ZookeeperNode::WatcherCallback* callback, std::string path, int type)
{
    if (type == ZK_SESSION_CLOSED_EVENT) {
        delete callback;
    } else {
        callback->Run(path, type);
    }
}

void ProcessChildrenWatcher(
        struct String_vector* sub_nodes,
        ZookeeperNode::ChildrenWatcherCallback* callback,
        std::string path,
        int type)
{
    std::vector<std::string> oldchildren;
    for (int i = 0; i < sub_nodes->count; ++i) {
        std::string child = sub_nodes->data[i];
        if (path == "/") {
            child = "/" + child;
        } else {
            child = path + "/" + child;
        }
        oldchildren.push_back(child);
    }
    deallocate_String_vector(sub_nodes);
    delete sub_nodes;
    if (type == ZK_SESSION_CLOSED_EVENT) {
        delete callback;
    } else {
        callback->Run(path, type, oldchildren);
    }
}

void ProcessLockWatcher(AutoResetEvent* sync_event, std::string path, int type)
{
    sync_event->Set();
}

void ProcessTryLockWatcher(ZookeeperSession* session,
        Closure<void, std::string, int>* callback,
        std::string path, int type)
{
    std::string::size_type pos = path.rfind("/");
    CHECK(pos != std::string::npos);
    std::string parent = path.substr(0, pos);

    // if session closed, give a notification.
    if (type == ZK_SESSION_CLOSED_EVENT) {
        callback->Run(parent, ZK_CLOSING);
        return;
    }

    // try to lock again.
    int error_code = 0;
    scoped_ptr<ZookeeperNode> node(session->Open(parent, &error_code));
    if (node == NULL) {
        callback->Run(parent, error_code);
    } else {
        node->AsyncLock(callback);
    }
}

} // anonymous namespace

// ZookeeperNode implementations.
int ZookeeperNode::Create(const std::string& data, int flags)
{
    scoped_array<char> buffer(new char[kMaxBufferLen]);
    // TODO(hsiaokangliu): Use ZOO_AUTH_IDS instead of ZOO_OPEN_ACL_UNSAFE
    int ret = m_session->Create(
            m_path.c_str(), data.data(), data.size(),
            &ZOO_OPEN_ACL_UNSAFE, flags, buffer.get(), kMaxBufferLen);
    if (ret == ZK_OK) {
        m_path.assign(buffer.get());
    }
    return ret;
}

int ZookeeperNode::RecursiveCreate(const std::string& data, int flags)
{
    if (!isValidPath(m_path.c_str(), flags, true)) {
        return ZK_INVALIDPATH;
    }
    if (m_path == "/") {
        return ZK_OK;
    }
    std::string::size_type pos = m_path.find_last_of('/');
    if (pos == std::string::npos) {
        return ZK_INVALIDPATH;
    }
    std::string parent = m_path.substr(0, pos);
    int ret = m_session->Exists(parent.c_str(), 0, NULL);
    if (ret == ZK_NONODE) {
        // parent not exists. create parent first.
        scoped_ptr<ZookeeperNode> node(m_session->Open(parent, &ret));
        if (node.get() == NULL) {
            return ret;
        }
        ret = node->RecursiveCreate();
    }
    // if create parent returns ZK_OK or ZK_NODEEXISTS, parent node exists now.
    if (ret == ZK_OK || ret == ZK_NODEEXISTS) {
        scoped_array<char> buffer(new char[kMaxBufferLen]);
        // TODO(hsiaokangliu): Use ZOO_AUTH_IDS instead of ZOO_OPEN_ACL_UNSAFE
        ret = m_session->Create(
            m_path.c_str(), data.data(), data.size(),
            &ZOO_OPEN_ACL_UNSAFE, flags, buffer.get(), kMaxBufferLen);
        if (ret == ZK_OK) {
            m_path.assign(buffer.get());
        }
    }
    return ret;
}

int ZookeeperNode::GetContentAsJson(std::vector<std::string>* contents)
{
    std::string value;
    Stat stat;
    int ret = GetContent(&value, &stat);
    if (ret != ZK_OK) {
        return ret;
    }
    std::ostringstream statstream;
    statstream << ",{\"node_stat\":";
    statstream << std::hex
        << "{\"czxid\":\"0x" << stat.czxid <<"\",\"mzxid\":\"0x" << stat.mzxid << "\""
        << ",\"pzxid\":\"0x" << stat.pzxid << "\",\"dataLength\":\""
        << std::dec <<  stat.dataLength << "\",\"numChildren\":\"" << stat.numChildren << "\""
        << ",\"version\":\"" << stat.version << "\",\"cversion\":\"" << stat.cversion << "\""
        << ",\"aversion\":\"" << stat.aversion  << "\",\"ctime\":\"" << stat.ctime << "\""
        << ",\"mtime\":\"" << stat.mtime << std::hex << "\",\"ephemeralOwner\":\"0x"
        << stat.ephemeralOwner
        << "\"}}";
    contents->push_back("{\"node_value\":\"" + value + "\"}");
    contents->push_back(statstream.str());

    return ZK_OK;
}

int ZookeeperNode::GetContent(std::string* contents, Stat* stat, WatcherCallback* callback)
{
    VLOG(1) << "Try get content of full path : " << m_path;
    scoped_array<char> buffer(new char[kMaxBufferLen]);
    int length = kMaxBufferLen;
    int error_code = 0;
    if (callback) {
        Closure<void, std::string, int>* cb = NewClosure(ProcessNodeWatcher, callback);
        error_code = m_session->GetWatcherManager()->AddGetWatcher(
                m_session, m_path, NodeWatcher, cb, buffer.get(), &length, stat);
        if (error_code != ZK_OK) {
            delete cb;
        }
    } else {
        error_code = m_session->Get(m_path.c_str(), 0, buffer.get(), &length, stat);
    }
    if (error_code == ZK_OK) {
        if (length > kMaxBufferLen) {
            return ZK_INVALIDDATA;
        } else {
            if (length < 0) {
                length = 0;
            }
            contents->assign(buffer.get(), length);
        }
    }
    return error_code;
}

int ZookeeperNode::SetContent(const std::string& value, int version)
{
    return m_session->Set(m_path.c_str(), value.data(), value.length(), version);
}

int ZookeeperNode::Exists(WatcherCallback* callback)
{
    int error_code = 0;
    if (callback) {
        Closure<void, std::string, int>* cb = NewClosure(ProcessNodeWatcher, callback);
        error_code = m_session->GetWatcherManager()->AddExistWatcher(
                m_session, m_path, NodeWatcher, cb, NULL);
        if (error_code != ZK_OK && error_code != ZK_NONODE) {
            delete cb;
        }
    } else {
        error_code = m_session->Exists(m_path.c_str(), 0, NULL);
    }
    return error_code;
}

int ZookeeperNode::Delete(int version)
{
    return m_session->Delete(m_path.c_str(), version);
}

int ZookeeperNode::RecursiveDelete()
{
    int retry_num = 0;
    int error_code = 0;
    while (retry_num <= ZookeeperSession::kMaxRetryNumber) {
        retry_num++;
        struct String_vector children = { 0, NULL };
        error_code = m_session->GetChildren(m_path.c_str(), 0, &children);
        if (error_code == ZK_NONODE) {
            LOG(WARNING) << "Delete node " << m_path << " returns NONODE, "
                << "maybe this node has been deleted.";
            deallocate_String_vector(&children);
            return ZK_OK;
        }
        if (error_code == ZK_OK) {
            for (int i = 0; i < children.count; ++i) {
                std::string child = m_path + "/" + std::string(children.data[i]);
                scoped_ptr<ZookeeperNode> node(m_session->Open(child, &error_code));
                if (node != NULL) {
                    error_code = node->RecursiveDelete();
                }
                if (error_code != ZK_OK) {
                    break;
                }
            }
        }
        deallocate_String_vector(&children);
        if (error_code == ZK_OK) {
            break;
        }
    }
    if (error_code == ZK_OK) {
        error_code = m_session->Delete(m_path.c_str(), -1);
    }
    return error_code;
}

int ZookeeperNode::GetChildContent(
        const std::string& child_name,
        std::string* contents,
        Stat* stat)
{
    std::string child;
    if (m_path == "/") {
        child = m_path + child_name;
    } else {
        child = m_path + "/" + child_name;
    }
    scoped_array<char> buffer(new char[kMaxBufferLen]);
    int length = kMaxBufferLen;
    int error_code = m_session->Get(child.c_str(), 0, buffer.get(), &length, stat);
    if (error_code == ZK_OK) {
        if (length > kMaxBufferLen) {
            return ZK_INVALIDDATA;
        } else {
            if (length < 0) {
                length = 0;
            }
            contents->assign(buffer.get(), length);
        }
    }
    return error_code;
}

int ZookeeperNode::SetChildContent(
        const std::string& child_name,
        const std::string& contents,
        int  version)
{
    if (static_cast<int>(contents.size()) >= kMaxBufferLen) {
        return ZK_INVALIDDATA;
    }
    std::string child;
    if (m_path == "/") {
        child = m_path + child_name;
    } else {
        child = m_path + "/" + child_name;
    }
    return m_session->Set(child.c_str(), contents.data(), contents.length(), version);
}


int ZookeeperNode::GetChildren(std::vector<std::string>* children,
        ChildrenWatcherCallback* callback)
{
    children->clear();
    int error_code = 0;
    if (callback) {
        struct String_vector* sub_nodes = new struct String_vector();
        WatcherCallback* cb = NewClosure(&ProcessChildrenWatcher, sub_nodes, callback);
        error_code = m_session->GetWatcherManager()->AddChildrenWatcher(
                m_session, m_path, NodeWatcher, cb, sub_nodes);
        if (error_code != ZK_OK) {
            deallocate_String_vector(sub_nodes);
            delete sub_nodes;
            delete cb;
            return error_code;
        }
        for (int i = 0; i < sub_nodes->count; ++i) {
            std::string child = sub_nodes->data[i];
            // Zookeeper c client has a bug: if you get children of '/',
            // you will get results like: //zk . We must deal with such a situation.
            if (m_path == "/") {
                child = "/" + child;
            } else {
                child = m_path + "/" + child;
            }
            children->push_back(child);
        }
    } else {
        struct String_vector sub_nodes = { 0, NULL };
        error_code = m_session->GetChildren(m_path.c_str(), 0, &sub_nodes);
        if (error_code != ZK_OK) {
            deallocate_String_vector(&sub_nodes);
            return error_code;
        }
        for (int i = 0; i < sub_nodes.count; ++i) {
            std::string child = sub_nodes.data[i];
            // Zookeeper c client has a bug: if you get children of '/',
            // you will get results like: //zk . We must deal with such a situation.
            if (m_path == "/") {
                child = "/" + child;
            } else {
                child = m_path + "/" + child;
            }
            children->push_back(child);
        }
        deallocate_String_vector(&sub_nodes);
    }
    return ZK_OK;
}

int ZookeeperNode::GetAcl(ZookeeperAcl* acls, Stat* stat)
{
    acls->Clear();
    struct ACL_vector acl_vector;
    int error_code = m_session->GetAcl(m_path.c_str(), &acl_vector, stat);
    if (error_code != ZK_OK) {
        return error_code;
    }
    for (int32_t i = 0; i < acl_vector.count; ++i) {
        struct ZKACL& acl = acl_vector.data[i];
        acls->AddEntry(acl.id.scheme, acl.id.id, acl.perms);
    }
    deallocate_ACL_vector(&acl_vector);
    return error_code;
}

int ZookeeperNode::Unlock()
{
    struct String_vector children = { 0, NULL };
    int error_code = m_session->GetChildren(m_path.c_str(), 0, &children);
    if (error_code == ZK_OK) {
        std::vector<std::string> lock_nodes;
        int length = strlen(kLockNodePrefix);
        for (int i = 0; i < children.count; i++) { // Find current session lock node.
            if (strncmp(children.data[i], kLockNodePrefix, length) == 0) {
                lock_nodes.push_back(children.data[i]);
            }
        }
        if (lock_nodes.size() > 0) {
            SortLockNodes(&lock_nodes);
            std::string prefix = kLockNodePrefix + m_session->GetId();
            if (StringStartsWith(lock_nodes[0], prefix)) {
                std::string path = m_path + "/" + lock_nodes[0];
                error_code = m_session->Delete(path.c_str());
            } else {
                error_code = ZK_NOAUTH;
            }
        }
    }
    deallocate_String_vector(&children);
    return error_code;
}

bool ZookeeperNode::IsLocked(int* error_code)
{
    int error_placeholder = 0;
    if (error_code == NULL)
        error_code = &error_placeholder;

    bool locked = false;
    struct String_vector children = { 0, NULL };
    *error_code = m_session->GetChildren(m_path.c_str(), 0, &children);
    if (*error_code == ZK_OK) {
        int length = strlen(kLockNodePrefix);
        for (int i = 0; i < children.count; ++i) {
            if (strncmp(kLockNodePrefix, children.data[i], length) == 0) {
                locked = true;
                break;
            }
        }
    }
    deallocate_String_vector(&children);
    return locked;
}

bool LockNodeCompare(const std::string& a, const std::string& b)
{
    const char* x = a.c_str();
    const char* y = b.c_str();
    return (strcmp(strrchr(x, '-') + 1, strrchr(y, '-') + 1) <= 0);
}

void ZookeeperNode::SortLockNodes(std::vector<std::string>* nodes)
{
    std::sort(nodes->begin(), nodes->end(), LockNodeCompare);
}

bool ZookeeperNode::FindOrCreateLockNode(std::string* node_name, int* error_code)
{
    node_name->clear();
    struct String_vector children = { 0, NULL };
    *error_code = m_session->GetChildren(m_path.c_str(), 0, &children);
    if (*error_code != ZK_OK) {
        deallocate_String_vector(&children);
        return false;
    }

    // Find if data of current session exists.
    for (int i = 0; i < children.count; ++i) {
        std::string prefix = kLockNodePrefix + m_session->GetId();
        if (StringStartsWith(children.data[i], prefix)) {
            *node_name = children.data[i];
            break;
        }
    }
    deallocate_String_vector(&children); // release resources.

    // No lock node created by current session
    if (node_name->empty()) {
        scoped_array<char> buffer(new char[kMaxBufferLen]);
        std::string node = m_path + "/" + kLockNodePrefix + m_session->GetId() + "-";
        *error_code = m_session->Create(
                node.c_str(),
                NULL,
                -1,
                &ZOO_OPEN_ACL_UNSAFE,
                ZOO_EPHEMERAL | ZOO_SEQUENCE, // create a ephemeral node with sequencial number
                buffer.get(),
                kMaxBufferLen);
        if (*error_code != ZK_OK && *error_code != ZK_NODEEXISTS) // failed to create a node.
            return false;
        char* name_pos = strrchr(buffer.get(), '/');
        if (name_pos != NULL) {
            node_name->assign(name_pos + 1); // node_prefix_client_id-ephemeralnum
        } else {
            *error_code = ZK_INVALIDDATA;
            return false;
        }
    }
    return true;
}

/// retval: -1 system error
/// retval: 0  no precursor
/// retval: 1  has a precursor, store in param 2: precursor
int ZookeeperNode::FindPrecursor(std::string* precursor, int* error_code)
{
    precursor->clear();
    struct String_vector children = { 0, NULL };
    *error_code = m_session->GetChildren(m_path.c_str(), 0, &children);
    if (*error_code != ZK_OK) {
        deallocate_String_vector(&children); // release memory.
        return -1;
    }

    std::vector<std::string> lock_nodes;
    int length = strlen(kLockNodePrefix);
    for (int i = 0; i < children.count; ++i) {
        if (strncmp(children.data[i], kLockNodePrefix, length) == 0) {
            lock_nodes.push_back(children.data[i]);
        }
    }
    deallocate_String_vector(&children);

    SortLockNodes(&lock_nodes);
    std::string prefix = kLockNodePrefix + m_session->GetId();
    for (size_t i = 0; i < lock_nodes.size(); ++i) {
        if (StringStartsWith(lock_nodes[i], prefix)) {
            if (i > 0) {
                // precursor found.
                *precursor = lock_nodes[i - 1];
                return 1;
            } else {
                // no precursor.
                return 0;
            }
        }
    }
    *error_code = ZK_NONODE;
    return -1;
}

int ZookeeperNode::Lock()
{
    int error_code = ZK_OK;
    std::string lock_node_name;
    if (!FindOrCreateLockNode(&lock_node_name, &error_code)) {
        return error_code;
    }

    // block until lock acquired.
    while (true) {
        std::string precursor;
        int ret = FindPrecursor(&precursor, &error_code);
        if (ret == 0) {
            // no precursor, acquire lock directly.
            return ZK_OK;
        } else if (ret == -1) {
            // system error.
            return error_code;
        } else {
            // has a precursor
            precursor = m_path + "/" + precursor;
            AutoResetEvent sync_event;
            // Set watcher on the precursor node.
            scoped_array<char> buffer(new char[kMaxBufferLen]);
            int length = kMaxBufferLen;
            WatcherCallback* callback = NewClosure(ProcessLockWatcher, &sync_event);
            error_code = m_session->GetWatcherManager()->AddGetWatcher(
                    m_session, precursor, NodeWatcher, callback, buffer.get(), &length, NULL);
            if (error_code == ZK_NONODE) {
                // The selected precursor maybe has been deleted.
                // go to next loop to see if there is another precursor.
                delete callback;
                continue;
            } else if (error_code != ZK_OK) {
                // error encounters
                delete callback;
                return error_code;
            }
            // Add watcher to precursor successfully. now we just wait...
            sync_event.Wait();
        }
    }
}

int ZookeeperNode::TryLock()
{
    int error_code = ZK_OK;
    std::string lock_node_name;
    if (!FindOrCreateLockNode(&lock_node_name, &error_code)) {
        return error_code;
    }

    std::string precursor;
    int ret = FindPrecursor(&precursor, &error_code);
    if (ret == 1) {
        return ZK_ALREADYLOCKED;
    }
    return error_code;
}

void ZookeeperNode::AsyncLock(Closure<void, std::string, int>* callback)
{
    CHECK_NOTNULL(callback);
    int error_code = ZK_OK;
    std::string lock_node_name;
    if (!FindOrCreateLockNode(&lock_node_name, &error_code)) {
        callback->Run(m_path, error_code);
        return;
    }

    while (true) {
        std::string precursor;
        int ret = FindPrecursor(&precursor, &error_code);
        if (ret == 0) {
            // no precursor, acquire lock directly.
            callback->Run(m_path, ZK_OK);
            return;
        } else if (ret == -1) {
            // system error.
            callback->Run(m_path, error_code);
            return;
        } else {
            // has a precursor
            precursor = m_path + "/" + precursor;
            // Set watcher on the precursor node.
            scoped_array<char> buffer(new char[kMaxBufferLen]);
            int length = kMaxBufferLen;
            WatcherCallback* cb = NewClosure(ProcessTryLockWatcher, m_session, callback);
            error_code = m_session->GetWatcherManager()->AddGetWatcher(
                    m_session, precursor, NodeWatcher, cb, buffer.get(), &length, NULL);
            // The selected precursor maybe has been deleted.
            // go to next loop to see if there is another precursor.
            if (error_code == ZK_NONODE) {
                delete cb;
                continue;
            } else if (error_code != ZK_OK) {
                // other errors. no retry.
                delete cb;
                callback->Run(m_path, error_code);
            }
            return;
        }
    }
}

} // namespace common
