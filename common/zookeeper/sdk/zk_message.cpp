// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author: Yubing Yin(yubingyin@tencent.com)

#include "common/zookeeper/sdk/zk_message.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "common/base/byte_order.h"
#include "common/zookeeper/sdk/zk_operation.h"
#include "common/zookeeper/sdk/zookeeper_errno.h"
#include "thirdparty/glog/logging.h"

using namespace std;

namespace common
{

namespace zookeeper
{

static const int kZKProtocolVersion = 0;
static const int kZKNotifyRequestSize = 64;
static const int kZKNotifyResponseSize = 40;
static const int kZKNotifyResponseSizeWithError = 44;

char* AllocateZeroInitBuffer(int count, int size)
{
    char* result = reinterpret_cast<char*>(calloc(count, size));
    CHECK_NOTNULL(result);
    return result;
}

char* AllocateZeroInitBuffer(int size)
{
    return AllocateZeroInitBuffer(1, size);
}

void FreeBuffer(const char* data)
{
    free(const_cast<char*>(data));
}

int AdaptErrorCode(int ret_code)
{
    CHECK_NE(-ENOMEM, ret_code);
    return ret_code < 0 ? kZKMarshallError : kZKSuccess;
}

template <class T>
char* SerializeNumber(char* data, T number)
{
    T number_net = ByteOrder::LocalToNet(number);
    memcpy(data, reinterpret_cast<char*>(&number_net), sizeof(number_net));
    return data + sizeof(number_net);
}

template <class T>
const char* DeserializeNumber(const char* data, T* number)
{
    memcpy(reinterpret_cast<char*>(number), data, sizeof(*number));
    ByteOrder::NetToLocal(number);
    return data + sizeof(*number);
}

char* SerializeString(char* data, char* string, int length)
{
    memcpy(data, string, length);
    return data + length;
}

const char* DeserializeString(const char* data, char* string, int length)
{
    memcpy(string, data, length);
    return data + length;
}

void DumpString(const std::string& str, char** result)
{
    *result = AllocateZeroInitBuffer(static_cast<int>(str.length()) + 1);
    memcpy(*result, str.data(), str.length());
}

void DumpStringVector(const std::vector<std::string>& strings, ::String_vector* result)
{
    result->count = static_cast<int32_t>(strings.size());
    result->data = reinterpret_cast<char**>(
        AllocateZeroInitBuffer(result->count, sizeof(result->data[0])));

    for (int i = 0; i < result->count; ++i) {
        DumpString(strings[i], &result->data[i]);
    }
}

void DumpBuffer(const std::string& str, ::buffer* buffer)
{
    buffer->len = static_cast<int32_t>(str.length());
    buffer->buff = AllocateZeroInitBuffer(buffer->len);
    memcpy(buffer->buff, str.data(), str.length());
}

void DumpAcl(const ZooKeeperAclEntry& acl, ::ZKACL* result)
{
    result->perms = acl.permissions;
    DumpString(acl.id.scheme, &result->id.scheme);
    DumpString(acl.id.id, &result->id.id);
}

void DumpAclVector(const ZooKeeperAclVector& acls, ::ACL_vector* result)
{
    std::vector<ZooKeeperAclEntry> entrys;
    acls.Dump(&entrys);
    result->count = static_cast<int32_t>(entrys.size());
    result->data = reinterpret_cast<ZKACL*>(AllocateZeroInitBuffer(result->count, sizeof(ZKACL)));

    for (int i = 0; i < result->count; ++i) {
        DumpAcl(entrys[i], &result->data[i]);
    }
}

void PackString(const char* str, std::string* result)
{
    result->assign(str);
}

void PackBuffer(const ::buffer& buffer, std::string* result)
{
    result->assign(buffer.buff, buffer.len);
}

void PackStringVector(const ::String_vector& strings, std::vector<std::string>* result)
{
    result->clear();
    result->resize(strings.count);

    for (int i = 0; i < strings.count; ++i) {
        PackString(strings.data[i], &(*result)[i]);
    }
}

void PackAclVector(const ::ACL_vector& acls, ZooKeeperAclVector* result)
{
    for (int i = 0; i < acls.count; ++i) {
        ::ZKACL& acl = acls.data[i];
        result->SetAcl(acl.id.scheme, acl.id.id, acl.perms);
    }
}

void Buffer::Free()
{
    if (m_data) {
        FreeBuffer(m_data);
        m_data = NULL;
        m_length = 0;
    }
}

NotifyRequest::NotifyRequest()
    : m_protocol_version(kZKProtocolVersion), m_last_txn_id(0),
      m_timeout(0), m_session_id(0), m_password_length(0), m_credential_length(0)
{
    memset(m_password, 0, sizeof(m_password));
    memset(m_credential, 0, sizeof(m_credential));
}

NotifyRequest::NotifyRequest(int32_t timeout, int64_t last_txn_id, const string& credential,
                             const Session* session)
    : m_protocol_version(kZKProtocolVersion), m_last_txn_id(last_txn_id),
      m_timeout(timeout), m_session_id(0), m_password_length(0),
      m_credential_length(static_cast<int32_t>(credential.length()))
{
    CHECK_EQ(kZKCredentialLength, m_credential_length);
    memcpy(m_credential, credential.data(), m_credential_length);

    if (session) {
        m_session_id = session->GetId();
        const string& password = session->GetPassword();
        m_password_length = static_cast<int32_t>(password.length());

        CHECK_EQ(kZKPasswordLength, m_password_length);
        memcpy(m_password, password.data(), m_password_length);
    } else {
        memset(m_password, 0, sizeof(m_password));
    }
}

int NotifyRequest::Serialize(Buffer* buffer)
{
    char* data = AllocateZeroInitBuffer(kZKNotifyRequestSize);

    char* p = data;
    p = SerializeNumber(p, m_protocol_version);
    p = SerializeNumber(p, m_last_txn_id);
    p = SerializeNumber(p, m_timeout);
    p = SerializeNumber(p, m_session_id);
    p = SerializeNumber(p, m_password_length);
    p = SerializeString(p, m_password, kZKPasswordLength);
    p = SerializeNumber(p, m_credential_length);
    p = SerializeString(p, m_credential, kZKCredentialLength);

    buffer->Set(data, kZKNotifyRequestSize);
    return kZKSuccess;
}

int NotifyRequest::Deserialize(const Buffer& buffer)
{
    if (buffer.GetLength() != kZKNotifyRequestSize) {
        return kZKMarshallError;
    }

    const char* p = buffer.GetData();
    p = DeserializeNumber(p, &m_protocol_version);
    p = DeserializeNumber(p, &m_last_txn_id);
    p = DeserializeNumber(p, &m_timeout);
    p = DeserializeNumber(p, &m_session_id);
    p = DeserializeNumber(p, &m_password_length);
    p = DeserializeString(p, m_password, kZKPasswordLength);
    p = DeserializeNumber(p, &m_credential_length);
    p = DeserializeString(p, m_credential, kZKCredentialLength);

    CHECK_EQ(kZKPasswordLength, m_password_length);
    CHECK_EQ(kZKCredentialLength, m_credential_length);

    return kZKSuccess;
}

int RequestHeader::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_RequestHeader(oa, "header", &m_request_header));
}

int RequestHeader::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_RequestHeader(ia, "header", &m_request_header));
}

SetWatchesRequest::SetWatchesRequest(int64_t last_txn_id)
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
    m_body.relativeZxid = last_txn_id;
}

int SetWatchesRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_SetWatches(oa, "req", &m_body));
}

int SetWatchesRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_SetWatches(ia, "req", &m_body));
}

void SetWatchesRequest::SetWatches(const std::vector<std::string>& data_watches,
                                   const std::vector<std::string>& exist_watches,
                                   const std::vector<std::string>& child_watches)
{
    DumpStringVector(data_watches, &m_body.dataWatches);
    DumpStringVector(exist_watches, &m_body.existWatches);
    DumpStringVector(child_watches, &m_body.childWatches);
}

void SetWatchesRequest::GetWatches(std::vector<std::string>* data_watches,
                                   std::vector<std::string>* exist_watches,
                                   std::vector<std::string>* child_watches) const
{
    PackStringVector(m_body.dataWatches, data_watches);
    PackStringVector(m_body.existWatches, exist_watches);
    PackStringVector(m_body.childWatches, child_watches);
}

AddAuthRequest::AddAuthRequest()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int AddAuthRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_AuthPacket(oa, "req", &m_body));
}

int AddAuthRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_AuthPacket(ia, "req", &m_body));
}

void AddAuthRequest::SetAuth(const std::string& scheme, const std::string& auth)
{
    DumpString(scheme, &m_body.scheme);
    DumpBuffer(auth, &m_body.auth);
}

void AddAuthRequest::GetAuth(std::string* scheme, std::string* auth) const
{
    PackString(m_body.scheme, scheme);
    PackBuffer(m_body.auth, auth);
}

GetDataRequest::GetDataRequest(bool watch)
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
    m_body.watch = watch ? 1 : 0;
}

int GetDataRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_GetDataRequest(oa, "req", &m_body));
}

int GetDataRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_GetDataRequest(ia, "req", &m_body));
}

void GetDataRequest::SetPath(const std::string& path)
{
    DumpString(path, &m_body.path);
}

void GetDataRequest::GetPath(std::string* path) const
{
    PackString(m_body.path, path);
}

SetDataRequest::SetDataRequest(int32_t version)
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
    m_body.version = version;
}

void SetDataRequest::SetData(const std::string& path, const std::string& data)
{
    DumpString(path, &m_body.path);
    DumpBuffer(data, &m_body.data);
}

void SetDataRequest::GetData(std::string* path, std::string* data) const
{
    PackString(m_body.path, path);
    PackBuffer(m_body.data, data);
}

int SetDataRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_SetDataRequest(oa, "req", &m_body));
}

int SetDataRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_SetDataRequest(ia, "req", &m_body));
}

CreateRequest::CreateRequest(int32_t flags)
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
    m_body.flags = flags;
}

int CreateRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_CreateRequest(oa, "req", &m_body));
}

int CreateRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_CreateRequest(ia, "req", &m_body));
}

void CreateRequest::SetDataAcl(const std::string& path, const std::string& data,
                               const ZooKeeperAclVector& acls)
{
    DumpString(path, &m_body.path);
    DumpBuffer(data, &m_body.data);
    DumpAclVector(acls, &m_body.acl);
}

void CreateRequest::GetDataAcl(std::string* path, std::string* data,
                               ZooKeeperAclVector* acls) const
{
    PackString(m_body.path, path);
    PackBuffer(m_body.data, data);
    PackAclVector(m_body.acl, acls);
}

DeleteRequest::DeleteRequest(int32_t version)
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
    m_body.version = version;
}

int DeleteRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_DeleteRequest(oa, "req", &m_body));
}

int DeleteRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_DeleteRequest(ia, "req", &m_body));
}

void DeleteRequest::SetPath(const std::string& path)
{
    DumpString(path, &m_body.path);
}

void DeleteRequest::GetPath(std::string* path) const
{
    PackString(m_body.path, path);
}

GetChildrenRequest::GetChildrenRequest(bool watch)
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
    m_body.watch = watch ? 1 : 0;
}

int GetChildrenRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_GetChildrenRequest(oa, "req", &m_body));
}

int GetChildrenRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_GetChildrenRequest(ia, "req", &m_body));
}

void GetChildrenRequest::SetPath(const std::string& path)
{
    DumpString(path, &m_body.path);
}

void GetChildrenRequest::GetPath(std::string* path) const
{
    PackString(m_body.path, path);
}

GetChildrenStatRequest::GetChildrenStatRequest(bool watch)
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
    m_body.watch = watch ? 1 : 0;
}

int GetChildrenStatRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_GetChildren2Request(oa, "req", &m_body));
}

int GetChildrenStatRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_GetChildren2Request(ia, "req", &m_body));
}

void GetChildrenStatRequest::SetPath(const std::string& path)
{
    DumpString(path, &m_body.path);
}

void GetChildrenStatRequest::GetPath(std::string* path) const
{
    PackString(m_body.path, path);
}

SyncRequest::SyncRequest()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int SyncRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_SyncRequest(oa, "req", &m_body));
}

int SyncRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_SyncRequest(ia, "req", &m_body));
}

void SyncRequest::SetPath(const std::string& path)
{
    DumpString(path, &m_body.path);
}

void SyncRequest::GetPath(std::string* path) const
{
    PackString(m_body.path, path);
}

GetAclRequest::GetAclRequest()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int GetAclRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_GetACLRequest(oa, "req", &m_body));
}

int GetAclRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_GetACLRequest(ia, "req", &m_body));
}

void GetAclRequest::SetPath(const std::string& path)
{
    DumpString(path, &m_body.path);
}

void GetAclRequest::GetPath(std::string* path) const
{
    PackString(m_body.path, path);
}

SetAclRequest::SetAclRequest(int32_t version)
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
    m_body.version = version;
}

int SetAclRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_SetACLRequest(oa, "req", &m_body));
}

int SetAclRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_SetACLRequest(ia, "req", &m_body));
}

void SetAclRequest::SetPathAcl(const std::string& path, const ZooKeeperAclVector& acls)
{
    DumpString(path, &m_body.path);
    DumpAclVector(acls, &m_body.acl);
}

void SetAclRequest::GetPathAcl(std::string* path, ZooKeeperAclVector* acls)
{
    PackString(m_body.path, path);
    PackAclVector(m_body.acl, acls);
}

ExistsRequest::ExistsRequest(bool watch)
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
    m_body.watch = watch ? 1 : 0;
}

int ExistsRequest::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_ExistsRequest(oa, "req", &m_body));
}

int ExistsRequest::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_ExistsRequest(ia, "req", &m_body));
}

void ExistsRequest::SetPath(const std::string& path)
{
    DumpString(path, &m_body.path);
}

void ExistsRequest::GetPath(std::string* path) const
{
    PackString(m_body.path, path);
}

NotifyResponse::NotifyResponse()
    : m_error_code(0), m_protocol_version(0), m_timeout(0), m_session_id(0),
      m_password_length(kZKPasswordLength)
{
    memset(m_password, 0, sizeof(m_password));
}

NotifyResponse::NotifyResponse(int32_t error_code, int32_t protocol_version, int timeout,
                               const Session& session)
    : m_error_code(error_code), m_protocol_version(protocol_version), m_timeout(timeout),
      m_password_length(kZKPasswordLength)
{
    m_session_id = session.GetId();
    CHECK_EQ(kZKPasswordLength, session.GetPassword().length());
    memcpy(m_password, session.GetPassword().data(), kZKPasswordLength);
}

int NotifyResponse::Serialize(Buffer* buffer)
{
    char* data = AllocateZeroInitBuffer(kZKNotifyResponseSizeWithError);

    char* p = data;
    p = SerializeNumber(p, m_protocol_version);
    p = SerializeNumber(p, m_timeout);
    p = SerializeNumber(p, m_session_id);
    p = SerializeNumber(p, m_password_length);
    p = SerializeString(p, m_password, kZKPasswordLength);
    p = SerializeNumber(p, m_error_code);

    buffer->Set(data, kZKNotifyResponseSizeWithError);
    return kZKSuccess;
}

int NotifyResponse::Deserialize(const Buffer& buffer)
{
    int length = buffer.GetLength();
    if (length != kZKNotifyResponseSize && length != kZKNotifyResponseSizeWithError) {
        return kZKMarshallError;
    }

    const char* p = buffer.GetData();
    p = DeserializeNumber(p, &m_protocol_version);
    p = DeserializeNumber(p, &m_timeout);
    p = DeserializeNumber(p, &m_session_id);
    p = DeserializeNumber(p, &m_password_length);
    p = DeserializeString(p, m_password, kZKPasswordLength);

    if (length == kZKNotifyResponseSizeWithError) {
        p = DeserializeNumber(p, &m_error_code);
    }

    CHECK_EQ(kZKPasswordLength, m_password_length);
    return kZKSuccess;
}

ResponseHeader::ResponseHeader()
{
    memset(reinterpret_cast<char*>(&m_response_header), 0, sizeof(m_response_header));
}

ResponseHeader::ResponseHeader(int32_t error_code, int32_t operation_id, int64_t txn_id)
{
    m_response_header.err = error_code;
    m_response_header.xid = operation_id;
    m_response_header.zxid = txn_id;
}

int ResponseHeader::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_ReplyHeader(oa, "req", &m_response_header));
}

int ResponseHeader::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_ReplyHeader(ia, "req", &m_response_header));
}

SetDataResponse::SetDataResponse()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int SetDataResponse::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_SetDataResponse(oa, "req", &m_body));
}

int SetDataResponse::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_SetDataResponse(ia, "req", &m_body));
}


SyncResponse::SyncResponse()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int SyncResponse::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_SyncResponse(oa, "req", &m_body));
}

int SyncResponse::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_SyncResponse(ia, "req", &m_body));
}

SetAclResponse::SetAclResponse()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int SetAclResponse::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_SetACLResponse(oa, "req", &m_body));
}

int SetAclResponse::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_SetACLResponse(ia, "req", &m_body));
}

WatcherEvent::WatcherEvent()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

WatcherEvent::WatcherEvent(int32_t type, int32_t state)
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
    m_body.type = type;
    m_body.state = state;
}

int WatcherEvent::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_WatcherEvent(oa, "req", &m_body));
}

int WatcherEvent::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_WatcherEvent(ia, "req", &m_body));
}

void WatcherEvent::SetPath(const std::string& path)
{
    DumpString(path, &m_body.path);
}

CreateResponse::CreateResponse()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int CreateResponse::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_CreateResponse(oa, "req", &m_body));
}

int CreateResponse::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_CreateResponse(ia, "req", &m_body));
}

void CreateResponse::SetPath(const std::string& path)
{
    DumpString(path, &m_body.path);
}

ExistsResponse::ExistsResponse()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int ExistsResponse::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_ExistsResponse(oa, "req", &m_body));
}

int ExistsResponse::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_ExistsResponse(ia, "req", &m_body));
}

GetDataResponse::GetDataResponse()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int GetDataResponse::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_GetDataResponse(oa, "req", &m_body));
}

int GetDataResponse::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_GetDataResponse(ia, "req", &m_body));
}

void GetDataResponse::SetData(const std::string& data)
{
    DumpBuffer(data, &m_body.data);
}

void GetDataResponse::GetData(std::string* data) const
{
    PackBuffer(m_body.data, data);
}

GetChildrenResponse::GetChildrenResponse()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int GetChildrenResponse::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_GetChildrenResponse(oa, "req", &m_body));
}

int GetChildrenResponse::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_GetChildrenResponse(ia, "req", &m_body));
}

void GetChildrenResponse::SetChildren(const std::vector<std::string>& children)
{
    DumpStringVector(children, &m_body.children);
}

void GetChildrenResponse::GetChildren(std::vector<std::string>* children) const
{
    PackStringVector(m_body.children, children);
}

GetChildrenStatResponse::GetChildrenStatResponse()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int GetChildrenStatResponse::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_GetChildren2Response(oa, "req", &m_body));
}

int GetChildrenStatResponse::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_GetChildren2Response(ia, "req", &m_body));
}

void GetChildrenStatResponse::SetChildren(const std::vector<std::string>& children)
{
    DumpStringVector(children, &m_body.children);
}

void GetChildrenStatResponse::GetChildren(std::vector<std::string>* children) const
{
    PackStringVector(m_body.children, children);
}

GetAclResponse::GetAclResponse()
{
    memset(reinterpret_cast<char*>(&m_body), 0, sizeof(m_body));
}

int GetAclResponse::Serialize(OutputStream* stream)
{
    ::oarchive* oa = stream->Get();
    return AdaptErrorCode(::serialize_GetACLResponse(oa, "req", &m_body));
}

int GetAclResponse::Deserialize(InputStream* stream)
{
    ::iarchive* ia = stream->Get();
    return AdaptErrorCode(::deserialize_GetACLResponse(ia, "req", &m_body));
}

void GetAclResponse::SetAcl(const ZooKeeperAclVector& acls)
{
    DumpAclVector(acls, &m_body.acl);
}

void GetAclResponse::GetAcl(ZooKeeperAclVector* acls) const
{
    PackAclVector(m_body.acl, acls);
}

} // namespace zookeeper

} // namespace common
