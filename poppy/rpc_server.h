// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>
// Eric Liu <ericliu@tencent.com>

#ifndef POPPY_RPC_SERVER_H
#define POPPY_RPC_SERVER_H

#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "common/base/export_variable.h"
#include "common/base/timed_stats.h"
#include "common/crypto/credential/verifier.h"
#include "common/net/http/http_server.h"
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/condition_variable.h"
#include "common/system/concurrency/mutex.h"
#include "common/system/concurrency/rwlock.h"
#include "common/system/timer/timer_manager.h"
#include "poppy/rpc_server_option_info.pb.h"
#include "poppy/stats_registry.h"
#include "thirdparty/ctemplate/template.h"
#include "thirdparty/protobuf/service.h"

namespace poppy {

// Max single RPC response message size, after serialze but before compress.
const int kMaxResponseSize = 32 * 1024 * 1024;

// using namespace common;

class RpcController;
class RpcServerHandler;
class TextConverter;

extern const char kRpcFormHttpPath[];
extern const char kStaticResourceHttpPath[];
extern const char kRpcServiceHttpPath[];

class RpcServer : public HttpServer
{
    friend class RpcServerHandler;
    friend void HandleServiceRequest(const RpcServerHandler* rpc_handler,
                                     const HttpRequest* http_request,
                                     HttpResponse* http_response,
                                     Closure<void>* done);

    friend void TextRequestComplete(
        int64_t begin_time,
        RpcServer* rpc_server,
        const google::protobuf::MethodDescriptor* method,
        RpcController* controller,
        google::protobuf::Message* request,
        google::protobuf::Message* response,
        const HttpRequest* http_request,
        HttpResponse* http_response,
        TextConverter* output_converter,
        Closure<void>* done);

    class ProcessingRequests
    {
    public:
        ProcessingRequests();
        void RegisterRequest(RpcController* rpc_controller);
        void UnregisterRequest(RpcController* rpc_controller);
        void WaitForEmpty();
        void Clear();
        int64_t Count() const;

    private:
#ifndef NDEBUG
        std::set<RpcController*> m_requests;
        mutable Mutex m_request_mutex;
        ConditionVariable m_all_requests_finish_cond;
#else
        Atomic<int64_t> m_request_count;
#endif
    };

public:
    // Pass in a net frame from external. If "own_net_frame" is true, the rpc
    // server would own the net frame and delete it in destructor.
    explicit RpcServer(
        netframe::NetFrame* net_frame,
        bool own_net_frame = false,
        common::CredentialVerifier* credential_verifier = NULL,
        const RpcServerOptions& options = RpcServerOptions());

    // The rpc server create and own a net frame internally.
    // If "server_threads" is 0, it means the number of logic cpus.
    explicit RpcServer(
        int server_threads = 0,
        common::CredentialVerifier* credential_verifier = NULL,
        const RpcServerOptions& options = RpcServerOptions());

    virtual ~RpcServer();

    bool Start(const std::string& server_address,
               SocketAddress* real_bind_addreess = NULL);

    // 等待所有的请求完成后停止服务器
    void Stop();

    // 快速停止服务器，不会等待所有的请求完成
    void QuickStop();

    virtual bool IsHealthy()
    {
        return true;
    }

    common::CredentialVerifier* GetCredentialVerifier()
    {
        return m_credential_verifier;
    }

    // Register a http handler on a specified path.
    // Return false if another handler already has been registered.
    // If a handler has been registered successfully, it will be taken over
    // by the handler manager and CANNOT be unregistered.
    void RegisterHandler(const std::string& path, HttpHandler* handler);

    // Add a link on the index page of poppy server pointing to the link.
    // "display_name" is the anchor text of the hyperlink.
    void AddLinkOnIndexPage(const std::string& path, const std::string& display_name);

    // Register an rpc service.
    // If a service has been registered successfully, it will be taken over
    // by the rpc server and will be deleted when the server destroys.
    void RegisterService(google::protobuf::Service* service);

    bool CallServiceMethod(google::protobuf::Service* service,
                           const google::protobuf::MethodDescriptor* method,
                           RpcController* controller,
                           const google::protobuf::Message* request,
                           google::protobuf::Message* response,
                           google::protobuf::Closure* done);

    size_t GetRequestMemoryUsage();
    size_t GetNetSendMemoryUsage();
    size_t GetNetReceiveMemoryUsage();

    const RpcStatsInfo& GetGlobalRpcStats() const;

private:
    void CommonConstruct();
    void ExportVariables();
    void InitCreditialVerifier();
    // Make this function private.
    void RegisterBuiltinHandlers();
    void RegisterBuiltinServices();
    void UnregisterAllServices();
    void HandleStatusRequest(const HttpRequest*, HttpResponse*, Closure<void>*);
    void HandleHealthRequest(const HttpRequest*, HttpResponse*, Closure<void>*);
    void HandleVarsRequest(const HttpRequest*, HttpResponse*, Closure<void>*);
    void HandleVersionRequest(const HttpRequest*, HttpResponse*, Closure<void>*);
    void HandleIndexRequest(const HttpRequest*, HttpResponse*, Closure<void>*);
    void HandleProfileRequest(const HttpRequest*, HttpResponse*, Closure<void>*);
    void HandleFlagsRequest(const HttpRequest*, HttpResponse*, Closure<void>*);
    void HandleGetFlagsRequest(const HttpRequest*, HttpResponse*);
    void HandleModifyFlagsRequest(const HttpRequest*, HttpResponse*);

    void ProfileComplete(const char* filename,
                         const HttpRequest* request,
                         HttpResponse* response,
                         Closure<void>* done,
                         uint64_t timer_id);

    void RegisterStaticResources();
    void FillConnectionsInfo(ctemplate::TemplateDictionary* dict);

    void RegisterRequest(RpcController* controller);
    void UnregisterRequest(RpcController* controller);

    void DoStop(bool wait_all_request_done);

    void IncreaseRequestMemorySize(size_t memory_size);
    void DecreaseRequestMemorySize(size_t memory_size);

    int64_t GetProcessingRequestCount() const;
    static void ParseFilterVars(const std::string& filter_value,
                                std::vector<std::string>* variables);
private:
    bool m_is_profiling;
    bool m_has_stopped;
    TimerManager m_timer_manager;
    common::CredentialVerifier* m_credential_verifier;
    RpcServerOptions m_options;
    RpcServerHandler* m_rpc_handler;

    Mutex m_displayed_links_mutex;
    ctemplate::TemplateDictionary m_displayed_links_dict;

    RWLock m_stop_rwlock;

    size_t m_request_memory_size;

    // Store stats information
    StatsRegistry m_stats_registry;

    double m_cpu_usage;
    uint64_t m_physical_memory_usage;
    uint64_t m_virtual_memory_usage;

    VariableExporter m_variable_exporter;
    ProcessingRequests m_processing_requests;
};

} // namespace poppy

#endif // POPPY_RPC_SERVER_H
