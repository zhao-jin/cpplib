// Copyright 2011, Tencent Inc.
// Author: Hangjun Ye <hansye@tencent.com>
// Xiaokang Liu <hsiaokangliu@tencent.com>
//
// The implementation of rpc controller.

#ifndef POPPY_RPC_CONTROLLER_H
#define POPPY_RPC_CONTROLLER_H

#include <list>
#include <string>

#include "common/base/closure.h"
#include "common/base/deprecate.h"
#include "common/base/stdint.h"
#include "common/system/net/socket_address.h"

#include "poppy/rpc_error_code.h"
#include "poppy/rpc_error_code_info.pb.h"
#include "poppy/rpc_option.pb.h"
#include "thirdparty/protobuf/service.h"

namespace poppy {

// using namespace common;

class RpcBuiltinService;
class RpcChannelImpl;
class RpcChannelSwig;
class RpcClientHandler;
class RpcControllerTest;
class RpcRequestQueue;
class RpcServerHandler;
class RpcServerSessionPool;
class RpcRequest;

// Per request call context, for both client and server side.
class RpcController : public google::protobuf::RpcController
{
    friend class RpcBuiltinService;
    friend class RpcChannelImpl;
    friend class RpcConnection;
    friend class RpcChannelSwig;
    friend class RpcClientHandler;
    friend class RpcControllerTest;
    friend class RpcHttpChannel;
    friend class RpcRequestQueue;
    friend class RpcServerHandler;
    friend class RpcServerSessionPool;
    friend class RpcRequest;
    friend void RpcControllerSetFailed(int, poppy::RpcController*);

public:
    RpcController();
    virtual ~RpcController();

    // Base class interfaces implementations.
    virtual void Reset();

    virtual void StartCancel();
    virtual bool IsCanceled() const
    {
        return m_canceled;
    }

    virtual void NotifyOnCancel(google::protobuf::Closure* callback)
    {
        m_cancel_listeners.push_back(callback);
    }

    virtual bool Failed() const
    {
        return m_error_code != RPC_SUCCESS;
    }

    virtual int ErrorCode() const
    {
        return m_error_code;
    }
    virtual std::string ErrorText() const;

    virtual void SetFailed(const std::string& reason)
    {
        SetFailed(RPC_ERROR_FROM_USER, reason);
    }

    virtual const SocketAddress& RemoteAddress() const
    {
        return m_remote_address;
    }

    virtual const std::string& Credential() const
    {
        return m_credential;
    }

    virtual const std::string& User() const
    {
        return m_user;
    }

    virtual const std::string& Role() const
    {
        return m_role;
    }

    virtual int64_t Time() const
    {
        return m_time;
    }

    // set expect timeout for rpc calla.
    // this option is permanent during each call,
    // if timeout is set to 0, actual timeout will be taken from proto options
    void SetTimeout(int64_t timeout_in_ms)
    {
        m_user_options.timeout = timeout_in_ms;
    }
    // If user set timeout implicitly, return the timeout set by user.
    // Otherwise return the default set by method/service.
    virtual int64_t Timeout() const
    {
        return m_user_options.timeout > 0 ?
                m_user_options.timeout : m_auto_options.timeout;
    }

    void SetRequestCompressType(int compress_type)
    {
        m_user_options.request_compress_type =
                static_cast<CompressType>(compress_type);
    }

    void SetResponseCompressType(int compress_type)
    {
        m_user_options.response_compress_type =
            static_cast<CompressType>(compress_type);
    }

    DEPRECATED_BY(RpcErrorCodeToString)
    static std::string ErrorCodeToString(int error_code)
    {
        return RpcErrorCodeToString(error_code);
    }

    /////////////////////////////////////////////////////////////////////////
    // the following interfaces are deprecated and will be removed in the
    // future, you should use the camel cased names

    DEPRECATED_BY(ErrorCode) int error_code() const { return ErrorCode(); }

    DEPRECATED_BY(User) const std::string& user() const { return User(); }

    DEPRECATED_BY(SetTimeout) void set_timeout(int64_t timeout_in_ms) { SetTimeout(timeout_in_ms); }

    DEPRECATED_BY(RequestCompressType)
    int request_compress_type() const { return RequestCompressType(); }

    DEPRECATED_BY(SetRequestCompressType)
    void set_request_compress_type(int compress_type)
    {
        SetRequestCompressType(compress_type);
    }

    DEPRECATED_BY(ResponseCompressType)
    int response_compress_type() const { return ResponseCompressType(); }

    DEPRECATED_BY(SetResponseCompressType)
    void set_response_compress_type(int compress_type)
    {
        SetResponseCompressType(compress_type);
    }

    virtual std::string Dump() const;

protected:
    void set_method_full_name(const std::string& method_full_name)
    {
        m_method_full_name = method_full_name;
    }

    void SetFailed(int error_code, const std::string& reason = "")
    {
        CHECK_NE(error_code, RPC_SUCCESS);
        m_error_code = error_code;
        m_reason = reason;
    }

private:
    std::string reason() const
    {
        return m_reason;
    }

    int64_t connection_id() const
    {
        return m_connection_id;
    }

    void set_connection_id(const int64_t connection_id_value)
    {
        m_connection_id = connection_id_value;
    }

    int64_t sequence_id() const
    {
        return m_sequence_id;
    }

    void set_sequence_id(const int64_t sequence_id_value)
    {
        m_sequence_id = sequence_id_value;
    }

    // sync or async call.
    void set_sync(bool sync)
    {
        m_sync = sync;
    }

    bool is_sync() const
    {
        return m_sync;
    }

    const std::string& method_full_name() const
    {
        return m_method_full_name;
    }

    void set_time(const int64_t time_in_ms)
    {
        m_time = time_in_ms;
    }

    void set_canceled(const bool canceled_value)
    {
        m_canceled = canceled_value;
    }

    // fail immediately when connection fails.
    void set_fail_immediately(bool fail_immediately)
    {
        m_fail_immediately = fail_immediately;
    }

    bool fail_immediately() const
    {
        return m_fail_immediately;
    }

    void set_remote_address(const SocketAddress& address)
    {
        m_remote_address = address;
    }

    void set_in_use(bool in_use)
    {
        m_in_use = in_use;
    }

    bool in_use() const
    {
        return m_in_use;
    }

    void set_credential(const std::string& credential)
    {
        m_credential = credential;
    }

    void set_user(const std::string& user)
    {
        m_user = user;
    }

    void set_role(const std::string& role)
    {
        m_role = role;
    }

    // all types are actually enum poppy::CompressType
    int RequestCompressType() const
    {
        return m_user_options.request_compress_type != CompressTypeAuto ?
                m_user_options.request_compress_type :
                m_auto_options.request_compress_type;
    }

    int ResponseCompressType() const
    {
        return m_user_options.response_compress_type != CompressTypeAuto ?
                m_user_options.response_compress_type :
                m_auto_options.response_compress_type;
    }

    // reset all internal state option except user options
    void InternalReset();

    // fill method name, timeout, compress type ... from MethodDescriptor
    void FillFromMethodDescriptor(const google::protobuf::MethodDescriptor* method);

    size_t MemoryUsage() const;

private:
    // Customized for our rpc implementation.
    int64_t m_time; // this time is set when sending request or recieving request
    int64_t m_connection_id;
    int64_t m_sequence_id;
    int m_error_code;

    // NOTE: m_in_use is only used in poppy component, don't touch it in Reset().
    bool m_canceled;
    bool m_fail_immediately; // if the connection fails, don't retry.
    bool m_in_use;
    bool m_sync;

    struct Options {
        int64_t timeout;
        CompressType request_compress_type;
        CompressType response_compress_type;
    };

    std::string m_method_full_name;
    // Required by google::protobuf::RpcController interface.
    std::string m_reason;
    std::list<google::protobuf::Closure*> m_cancel_listeners;
    SocketAddressInet m_remote_address;

    // Only use in server client
    std::string m_credential;
    std::string m_user;
    std::string m_role;

    // user options are set by user, are permanent during each call,
    // and are priority than internal options
    Options m_user_options;

    // auto set/reset according to proto options during each call
    Options m_auto_options;
};

} // namespace poppy

#endif // POPPY_RPC_CONTROLLER_H
