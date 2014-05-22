// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author: Yubing Yin(yubingyin@tencent.com)

#ifndef COMMON_ZOOKEEPER_SDK_ZK_MESSAGE_H
#define COMMON_ZOOKEEPER_SDK_ZK_MESSAGE_H

#include <string>
#include <vector>

#include "common/zookeeper/sdk/zookeeper.h"
#include "common/zookeeper/sdk/zookeeper_errno.h"
#include "thirdparty/zookeeper-3.3.3/src/c/generated/zookeeper.jute.h"
#include "thirdparty/zookeeper-3.3.3/src/c/include/recordio.h"

namespace common
{

namespace zookeeper
{

static const int kZKPasswordLength = 16;
static const int kZKCredentialLength = 16;

class Buffer
{
public:
    Buffer() : m_data(NULL), m_length(0) {}

    Buffer(const char* data, int length) : m_data(data), m_length(length) {}

    ~Buffer()
    {
        Free();
    }

    void Set(const char* data, int length)
    {
        Free();
        m_data = data;
        m_length = length;
    }

    const char* GetData() const
    {
        return m_data;
    }

    int GetLength() const
    {
        return m_length;
    }

    void Free();

private:
    const char* m_data;
    int         m_length;
};

class InputStream
{
public:
    InputStream(char* data, int length)
    {
        m_stream = ::create_buffer_iarchive(data, length);
    }

    ~InputStream()
    {
        ::close_buffer_iarchive(&m_stream);
    }

    bool IsOpen() const
    {
        return m_stream != NULL;
    }

    ::iarchive* Get()
    {
        return m_stream;
    }

private:
    ::iarchive* m_stream;
};

class OutputStream
{
public:
    OutputStream() : m_finish(false)
    {
        m_stream = ::create_buffer_oarchive();
    }

    ~OutputStream()
    {
        if (!m_finish) {
            ::close_buffer_oarchive(&m_stream, 1);
        }
    }

    bool IsOpen() const
    {
        return m_stream != NULL;
    }

    bool Finish(Buffer* buffer)
    {
        if (m_finish) {
            return false;
        }

        m_finish = true;
        buffer->Set(::get_buffer(m_stream), ::get_buffer_len(m_stream));
        ::close_buffer_oarchive(&m_stream, 0);
        return true;
    }

    ::oarchive* Get()
    {
        return m_stream;
    }

private:
    bool        m_finish;
    ::oarchive* m_stream;
};

class NotifyRequest
{
public:
    NotifyRequest();

    NotifyRequest(int32_t timeout, int64_t last_txn_id, const std::string& credential,
                  const Session* session = NULL);

    int Serialize(Buffer* buffer);

    int Deserialize(const Buffer& buffer);

    int32_t GetProtocolVersion() const
    {
        return m_protocol_version;
    }

    int64_t GetLastTxnId() const
    {
        return m_last_txn_id;
    }

    int32_t GetTimeout() const
    {
        return m_timeout;
    }

    void GetSession(Session* session) const
    {
        return session->Set(m_session_id, std::string(m_password, kZKPasswordLength));
    }

    void GetCredential(std::string* credential) const
    {
        credential->assign(m_credential, kZKCredentialLength);
    }

private:
    int32_t     m_protocol_version;
    int64_t     m_last_txn_id;
    int32_t     m_timeout;
    int64_t     m_session_id;
    int32_t     m_password_length;
    char        m_password[kZKPasswordLength];
    int32_t     m_credential_length;
    char        m_credential[kZKCredentialLength];
};

class RequestHeader
{
public:
    RequestHeader()
    {
        m_request_header.xid = 0;
        m_request_header.type = 0;
    }

    RequestHeader(int32_t operation_id, int32_t operation_type)
    {
        m_request_header.xid = operation_id;
        m_request_header.type = operation_type;
    }

    ~RequestHeader()
    {
        ::deallocate_RequestHeader(&m_request_header);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    int32_t GetOperationId() const
    {
        return m_request_header.xid;
    }

    int32_t GetOperationType() const
    {
        return m_request_header.type;
    }

private:
    ::RequestHeader m_request_header;
};

class SetWatchesRequest
{
public:
    explicit SetWatchesRequest(int64_t last_txn_id = 0);

    ~SetWatchesRequest()
    {
        ::deallocate_SetWatches(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetWatches(const std::vector<std::string>& data_watches,
                    const std::vector<std::string>& exist_watches,
                    const std::vector<std::string>& child_watches);

    int64_t GetLastTxnId() const
    {
        return m_body.relativeZxid;
    }

    void GetWatches(std::vector<std::string>* data_watches,
                    std::vector<std::string>* exist_watches,
                    std::vector<std::string>* child_watches) const;

private:
    ::SetWatches m_body;
};

class AddAuthRequest
{
public:
    AddAuthRequest();

    ~AddAuthRequest()
    {
        ::deallocate_AuthPacket(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetAuth(const std::string& scheme, const std::string& auth);

    void GetAuth(std::string* scheme, std::string* auth) const;

private:
    ::AuthPacket m_body;
};

class GetDataRequest
{
public:
    explicit GetDataRequest(bool watch = false);

    ~GetDataRequest()
    {
        ::deallocate_GetDataRequest(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetPath(const std::string& path);

    void GetPath(std::string* path) const;

private:
    ::GetDataRequest m_body;
};

class SetDataRequest
{
public:
    explicit SetDataRequest(int32_t version = -1);

    ~SetDataRequest()
    {
        ::deallocate_SetDataRequest(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetData(const std::string& path, const std::string& data);

    void GetData(std::string* path, std::string* data) const;

private:
    ::SetDataRequest m_body;
};

class CreateRequest
{
public:
    explicit CreateRequest(int32_t flags = 0);

    ~CreateRequest()
    {
        ::deallocate_CreateRequest(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetDataAcl(const std::string& path, const std::string& data,
                    const ZooKeeperAclVector& acls);

    void GetDataAcl(std::string* path, std::string* data, ZooKeeperAclVector* acls) const;

private:
    ::CreateRequest m_body;
};

class DeleteRequest
{
public:
    explicit DeleteRequest(int32_t version = -1);

    ~DeleteRequest()
    {
        ::deallocate_DeleteRequest(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetPath(const std::string& path);

    void GetPath(std::string* path) const;

private:
    ::DeleteRequest m_body;
};

class GetChildrenRequest
{
public:
    explicit GetChildrenRequest(bool watch = false);

    ~GetChildrenRequest()
    {
        ::deallocate_GetChildrenRequest(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetPath(const std::string& path);

    void GetPath(std::string* path) const;

private:
    ::GetChildrenRequest m_body;
};

class GetChildrenStatRequest
{
public:
    explicit GetChildrenStatRequest(bool watch = false);

    ~GetChildrenStatRequest()
    {
        ::deallocate_GetChildren2Request(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetPath(const std::string& path);

    void GetPath(std::string* path) const;

private:
    ::GetChildren2Request m_body;
};

class SyncRequest
{
public:
    SyncRequest();

    ~SyncRequest()
    {
        ::deallocate_SyncRequest(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetPath(const std::string& path);

    void GetPath(std::string* path) const;

private:
    ::SyncRequest m_body;
};

class GetAclRequest
{
public:
    GetAclRequest();

    ~GetAclRequest()
    {
        ::deallocate_GetACLRequest(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetPath(const std::string& path);

    void GetPath(std::string* path) const;

private:
    ::GetACLRequest m_body;
};

class SetAclRequest
{
public:
    explicit SetAclRequest(int32_t version = -1);

    ~SetAclRequest()
    {
        ::deallocate_SetACLRequest(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetPathAcl(const std::string& path, const ZooKeeperAclVector& acls);

    void GetPathAcl(std::string* path, ZooKeeperAclVector* acls);

private:
    ::SetACLRequest m_body;
};

class ExistsRequest
{
public:
    explicit ExistsRequest(bool watch = false);

    ~ExistsRequest()
    {
        ::deallocate_ExistsRequest(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetPath(const std::string& path);

    void GetPath(std::string* path) const;

private:
    ::ExistsRequest m_body;
};

class NotifyResponse
{
public:
    NotifyResponse();

    NotifyResponse(int32_t error_code, int32_t protocol_version, int timeout,
                   const Session& session);

    int Serialize(Buffer* buffer);

    int Deserialize(const Buffer& buffer);

    int32_t GetTimeout() const
    {
        return m_timeout;
    }

    void GetSession(Session* session) const
    {
        return session->Set(m_session_id, std::string(m_password, kZKPasswordLength));
    }

    int32_t ErrorCode()
    {
        return m_error_code;
    }

private:
    int32_t     m_error_code;
    int32_t     m_protocol_version;
    int32_t     m_timeout;
    int64_t     m_session_id;
    char        m_password[kZKPasswordLength];
    int32_t     m_password_length;
};

class ResponseHeader
{
public:
    ResponseHeader();

    ResponseHeader(int32_t error_code, int32_t operation_id, int64_t txn_id);

    ~ResponseHeader()
    {
        ::deallocate_ReplyHeader(&m_response_header);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    int32_t ErrorCode() const
    {
        return m_response_header.err;
    }

    int32_t GetOperationId() const
    {
        return m_response_header.xid;
    }

    int64_t GetTxnId() const
    {
        return m_response_header.zxid;
    }

private:
    ::ReplyHeader m_response_header;
};

class SetDataResponse
{
public:
    SetDataResponse();

    ~SetDataResponse()
    {
        ::deallocate_SetDataResponse(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetStat(const Stat& stat)
    {
        stat.Get(&m_body.stat);
    }

    void GetStat(Stat* stat) const
    {
        stat->Set(m_body.stat);
    }

private:
    ::SetDataResponse m_body;
};

class SyncResponse
{
public:
    SyncResponse();

    ~SyncResponse()
    {
        ::deallocate_SyncResponse(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

private:
    ::SyncResponse m_body;
};

class SetAclResponse
{
public:
    SetAclResponse();

    ~SetAclResponse()
    {
        ::deallocate_SetACLResponse(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetStat(const Stat& stat)
    {
        stat.Get(&m_body.stat);
    }

    void GetStat(Stat* stat) const
    {
        stat->Set(m_body.stat);
    }

private:
    ::SetACLResponse m_body;
};

class WatcherEvent
{
public:
    WatcherEvent();

    WatcherEvent(int32_t type, int32_t state);

    ~WatcherEvent()
    {
        ::deallocate_WatcherEvent(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    int32_t GetEventType() const
    {
        return m_body.type;
    }

    int32_t GetSessionState() const
    {
        return m_body.state;
    }

    void GetPath(std::string* path) const
    {
        path->assign(m_body.path);
    }

    void SetPath(const std::string& path);

private:
    ::WatcherEvent m_body;
};

class CreateResponse
{
public:
    CreateResponse();

    ~CreateResponse()
    {
        ::deallocate_CreateResponse(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void GetPath(std::string* path) const
    {
        path->assign(m_body.path);
    }

    void SetPath(const std::string& path);

private:
    ::CreateResponse m_body;
};

class ExistsResponse
{
public:
    ExistsResponse();

    ~ExistsResponse()
    {
        ::deallocate_ExistsResponse(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void GetStat(Stat* stat) const
    {
        stat->Set(m_body.stat);
    }

    void SetStat(const Stat& stat)
    {
        stat.Get(&m_body.stat);
    }

private:
    ::ExistsResponse m_body;
};

class GetDataResponse
{
public:
    GetDataResponse();

    ~GetDataResponse()
    {
        ::deallocate_GetDataResponse(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void SetData(const std::string& data);

    void GetData(std::string* data) const;

    void GetStat(Stat* stat) const
    {
        stat->Set(m_body.stat);
    }

    void SetStat(const Stat& stat)
    {
        stat.Get(&m_body.stat);
    }

private:
    ::GetDataResponse m_body;
};

class GetChildrenResponse
{
public:
    GetChildrenResponse();

    ~GetChildrenResponse()
    {
        ::deallocate_GetChildrenResponse(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void GetChildren(std::vector<std::string>* children) const;

    void SetChildren(const std::vector<std::string>& children);

private:
    ::GetChildrenResponse m_body;
};

class GetChildrenStatResponse
{
public:
    GetChildrenStatResponse();

    ~GetChildrenStatResponse()
    {
        ::deallocate_GetChildren2Response(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void GetChildren(std::vector<std::string>* children) const;

    void SetChildren(const std::vector<std::string>& children);

    void GetStat(Stat* stat) const
    {
        stat->Set(m_body.stat);
    }

    void SetStat(const Stat& stat)
    {
        stat.Get(&m_body.stat);
    }

private:
    ::GetChildren2Response m_body;
};

class GetAclResponse
{
public:
    GetAclResponse();

    ~GetAclResponse()
    {
        ::deallocate_GetACLResponse(&m_body);
    }

    int Serialize(OutputStream* stream);

    int Deserialize(InputStream* stream);

    void GetAcl(ZooKeeperAclVector* acls) const;

    void SetAcl(const ZooKeeperAclVector& acls);

    void GetStat(Stat* stat) const
    {
        stat->Set(m_body.stat);
    }

    void SetStat(const Stat& stat)
    {
        stat.Get(&m_body.stat);
    }

private:
    ::GetACLResponse m_body;
};

} // namespace zookeeper

} // namespace common

#endif // COMMON_ZOOKEEPER_SDK_ZK_MESSAGE_H
