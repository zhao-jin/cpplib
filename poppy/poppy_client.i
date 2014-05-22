%module(directors="1") poppy_client

%{
#undef __DEPRECATED

#ifdef SWIGPHP
// php header file zend_API.h defined a macro named add_method
// CPP error replaced add_method in header file thirdparty/google/protobuf/descriptor.pb.h
#undef add_method
// php header file zend.h defined a macro named SUCCESS
// CPP error replaced SUCCESS in header file common/net/http/http_message.h
#undef SUCCESS
#endif

#include "poppy/rpc_channel_swig.h"
#include "poppy/rpc_client.h"
#include "poppy/rpc_controller_swig.h"

#ifdef SWIGPHP
#define SUCCESS 0
#define add_method(arg, key, method) add_assoc_function((arg), (key), (method))
#endif

typedef std::string binary_string;
%}

%include "stdint.i"
%include "std_string.i"

#ifdef SWIGJAVA
%include "binary_string.i"
%apply const binary_string& { const std::string& response_data,
                              const std::string& request_data };
#else
%apply std::string { binary_string };
%apply const std::string & { const binary_string & };
#endif

%feature("director") poppy::ResponseHandler;
#ifdef SWIGJAVA
SWIG_DIRECTOR_OWNED(poppy::ResponseHandler)
#endif

namespace poppy
{

class RpcControllerSwig
{
public:
    RpcControllerSwig();
    virtual ~RpcControllerSwig();

    void Reset();
    bool Failed() const;
    std::string ErrorText() const;
    int ErrorCode() const;
    std::string RemoteAddressString() const;
    void SetTimeout(int64_t timeout);
    void SetRequestCompressType(int compress_type);
    void SetResponseCompressType(int compress_type);

    int64_t Timeout() const;
    void set_method_full_name(const std::string& method_full_name);
    void SetFailed(int error_code, std::string reason = "");
};

class RpcClient
{
public:
    explicit RpcClient(int num_threads = 0);
    virtual ~RpcClient();
};

class ResponseHandler
{
public:
    virtual ~ResponseHandler();
    virtual void Run(const std::string& response_data) = 0;

protected:
    ResponseHandler();
};

class RpcChannelSwig
{
public:
    RpcChannelSwig(RpcClient* rpc_client, const std::string& name);
    virtual ~RpcChannelSwig();

    binary_string RawCallMethod(RpcControllerSwig* rpc_controller,
                                const std::string& request_data,
                                ResponseHandler* done);
};

} // namespace poppy

