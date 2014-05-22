// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>
// Eric Liu <ericliu@tencent.com>

#include "poppy/rpc_server.h"

#include <errno.h>
#include <map>
#include <utility>
#include <vector>
#include "common/base/array_size.h"
#include "common/base/binary_version.h"
#include "common/base/export_variable.h"
#include "common/base/static_resource.h"
#include "common/base/string/string_number.h"
#include "common/net/http/http_stats.h"
#include "common/net/http/http_time.h"
#include "common/net/uri/cgi_params.h"
#include "common/system/concurrency/atomic/atomic.h"
#include "common/system/concurrency/thread.h"
#include "common/system/cpu/cpu_usage.h"
#include "common/system/io/textfile.h"
#include "common/system/memory/mem_usage.h"
#include "common/system/process.h"

#include "poppy/rpc_builtin_service.h"
#include "poppy/rpc_form.h"
#include "poppy/rpc_handler.h"
#include "poppy/rpc_text.h"
#include "poppy/set_typhoon_tos_flag.h"
#include "poppy/static_resource.h"

#include "thirdparty/ctemplate/template.h"
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/jsoncpp/reader.h"
#include "thirdparty/jsoncpp/value.h"
#include "thirdparty/perftools/profiler.h"
#include "thirdparty/protobuf/descriptor.h"

#ifdef _WIN32
#include <process.h>
#endif

namespace poppy {

// using namespace common;

static const char kPoppyHttpPath[] = "/poppy";
static const char kVarsHttpPath[] = "/vars";
static const char kStatusHttpPath[] = "/status";
static const char kHealthHttpPath[] = "/health";
static const char kVersionHttpPath[] = "/version";
static const char kFlagsHttpPath[] = "/flags";
static const char kProfileHttpPath[] = "/pprof/profile";

extern const char kRpcFormHttpPath[] = "/rpc/form";
extern const char kStaticResourceHttpPath[] = "/rpc/form/static/";
extern const char kRpcServiceHttpPath[] = "/rpc";

#ifdef NDEBUG

RpcServer::ProcessingRequests::ProcessingRequests()
{
    m_request_count = 0;
}

void RpcServer::ProcessingRequests::RegisterRequest(RpcController* controller)
{
    ++m_request_count;
}

void RpcServer::ProcessingRequests::UnregisterRequest(RpcController* controller)
{
    --m_request_count;
}

int64_t RpcServer::ProcessingRequests::Count() const
{
    return m_request_count;
}

void RpcServer::ProcessingRequests::WaitForEmpty()
{
    while (m_request_count) {
        LOG(WARNING) << m_request_count << " requests is not done yet";
        ThisThread::Sleep(1000);
    }
    LOG(INFO) << "controller's set is empty now.";
}

void RpcServer::ProcessingRequests::Clear()
{
    m_request_count = 0;
}

#else

RpcServer::ProcessingRequests::ProcessingRequests()
{
}

void RpcServer::ProcessingRequests::RegisterRequest(RpcController* controller)
{
    MutexLocker locker(m_request_mutex);
    CHECK_EQ(m_requests.insert(controller).second, true);
}

void RpcServer::ProcessingRequests::UnregisterRequest(RpcController* controller)
{
    CHECK(controller != NULL) << "controller should not be NULL";
    MutexLocker locker(m_request_mutex);
    CHECK_EQ(m_requests.erase(controller), 1U);
    if (m_requests.empty()) {
        m_all_requests_finish_cond.Signal();
    }
}

int64_t RpcServer::ProcessingRequests::Count() const
{
    MutexLocker locker(m_request_mutex);
    return m_requests.size();
}

void RpcServer::ProcessingRequests::WaitForEmpty()
{
    MutexLocker locker(m_request_mutex);
    LOG(INFO) << "wait for controller's set become empty.";
    while (!m_requests.empty()) {
        if (!m_all_requests_finish_cond.Wait(m_request_mutex, 10000)) {
            LOG(WARNING) << m_requests.size() << " requests is not done yet";
            for (std::set<RpcController*>::iterator i = m_requests.begin();
                 i != m_requests.end(); ++i) {
                VLOG(3) << "Pending " << (*i)->Dump();
            }
        }
    }
    LOG(INFO) << "controller's set is empty now.";
}

void RpcServer::ProcessingRequests::Clear()
{
    MutexLocker locker(m_request_mutex);
    m_requests.clear();
}

#endif // NDEBUG

//////////////////////////////////////////////////////////////////

RpcServer::RpcServer(
    netframe::NetFrame* net_frame,
    bool own_net_frame,
    common::CredentialVerifier* credential_verifier,
    const RpcServerOptions& options
) :
    HttpServer(net_frame, own_net_frame),
    m_timer_manager("server_profile_timer"),
    m_credential_verifier(credential_verifier),
    m_options(options),
    m_displayed_links_dict("displayed links dict")
{
    CommonConstruct();
}

RpcServer::RpcServer(
    int server_threads,
    common::CredentialVerifier* credential_verifier,
    const RpcServerOptions& options
) :
    HttpServer(server_threads),
    m_timer_manager("server_profile_timer"),
    m_credential_verifier(credential_verifier),
    m_options(options),
    m_displayed_links_dict("displayed links dict")
{
    CommonConstruct();
}

void RpcServer::CommonConstruct()
{
    if (m_options.has_tos()) {
        CHECK(m_options.tos() > 0 && m_options.tos() < 256)
            << "Invalid tos option value";
    }

    m_is_profiling = false;
    m_has_stopped = false;
    m_rpc_handler = NULL;
    m_request_memory_size = 0;
    m_cpu_usage = 0;
    m_physical_memory_usage = 0;
    m_virtual_memory_usage = 0;

    SetTyphoonTosIfNecessary();

    ExportVariables();
    InitCreditialVerifier();

    RegisterBuiltinHandlers();
    RegisterBuiltinServices();
}

RpcServer::~RpcServer()
{
    Stop();

    UnregisterAllServices();
}

bool RpcServer::Start(const std::string& server_address,
                      SocketAddress* real_bind_address)
{
    if (AtomicGet(&m_has_stopped)) {
        DCHECK(false) << "RpcServer has already been stopped.";
    }
    netframe::NetFrame::EndPointOptions endpoint_options;
    if (m_options.has_tos()) {
        endpoint_options.Priority(m_options.tos());
    }
    return HttpServer::Start(server_address, endpoint_options, real_bind_address);
}

void RpcServer::Stop()
{
    DoStop(true);
}

void RpcServer::QuickStop()
{
    DoStop(false);
}

void RpcServer::DoStop(bool wait_all_request_done)
{
    if (AtomicExchange(&m_has_stopped, true)) {
        VLOG(3) << "RpcServer has already been stopped.";
        return;
    }

    // Make sure no method is calling by other threads
    {
        RWLock::WriterLocker locker(m_stop_rwlock);
    }

    if (wait_all_request_done) {
        m_processing_requests.WaitForEmpty();
    } else {
        m_processing_requests.Clear();
    }

    // TODO(simonwang): should add shutdown functionality to HttpServer
    HttpServer::Stop();

    m_request_memory_size = 0;
}

void RpcServer::ExportVariables()
{
#ifndef _WIN32
    m_variable_exporter.Export("start_time", &ThisProcess::StartTime);
    m_variable_exporter.Export("elapsed_time", &ThisProcess::ElapsedTime);
    m_variable_exporter.Export("processing_requests", this,
                               &RpcServer::GetProcessingRequestCount);
    m_variable_exporter.Export("cpu_usage", &m_cpu_usage);
    m_variable_exporter.Export("process_name", &ThisProcess::BinaryPath);
    m_variable_exporter.Export("physical_memory_usage",
                               &m_physical_memory_usage);
    m_variable_exporter.Export("virtual_memory_usage",
                               &m_virtual_memory_usage);
#endif
}

void RpcServer::InitCreditialVerifier()
{
    if (m_credential_verifier == NULL) {
        m_credential_verifier = common::CredentialVerifier::DefaultInstance();
    }
    if (m_credential_verifier == NULL) {
        m_credential_verifier = &common::DummyCredentialVerifier::Instance();
    }
}

#define STATIC_RESOURCE_PAIR(path, name) \
    std::make_pair("/rpc/form/static/" path, STATIC_RESOURCE(poppy_static_##name))

void RpcServer::RegisterStaticResources()
{
    const std::pair<const char*, StringPiece> static_resources[] = {
        STATIC_RESOURCE_PAIR("jquery-1.4.2.min.js", jquery_1_4_2_min_js),
        STATIC_RESOURCE_PAIR("jquery.json-2.2.min.js", jquery_json_2_2_min_js),
        STATIC_RESOURCE_PAIR("forms.js", forms_js),
        STATIC_RESOURCE_PAIR("flags.js", flags_js)
    };

    for (size_t i = 0; i < ARRAY_SIZE(static_resources); ++i) {
        HttpServer::RegisterStaticResource(static_resources[i].first,
                                           static_resources[i].second);
    }

    HttpServer::RegisterStaticResource("/favicon.ico",
                                       STATIC_RESOURCE(poppy_static_favicon_ico),
                                       HttpHeaders().Add("Content-Type", "image/x-icon"));
}

void RpcServer::HandleStatusRequest(const HttpRequest* request,
                                    HttpResponse* response,
                                    Closure<void>* done)
{
    ctemplate::TemplateDictionary dict("data");

    dict.SetValue("PROCESS_NAME", ThisProcess::BinaryPath());

    // Get process start time, elapsed time.
    time_t start_time = ThisProcess::StartTime();
    if (start_time % 2 == 1)
        start_time--;
    struct tm tm_start;
#ifndef _WIN32
    localtime_r(&start_time, &tm_start);
#endif
    char str[256];
    strftime(str, 256, "%Y-%m-%d %H:%M:%S", &tm_start);
    dict.SetValue("START_TIME", str);
    dict.SetIntValue("ELAPSED_TIME", ThisProcess::ElapsedTime());

    // Get cpu, mem info.
    int32_t pid = getpid();
    double cpu = 0;
    uint64_t vm_size, mem_size;
    size_t request_memory_size;
    size_t netframe_send_memory_size;
    size_t netframe_receive_memory_size;
    int64_t request_count;
    if (GetCpuUsageSinceLastCall(pid, &cpu)) {
        dict.SetFormattedValue("CPU_USAGE_PERCENT", "%.2f", cpu);
        dict.ShowSection("CPU_SUCCESS");
    } else {
        dict.ShowSection("CPU_ERROR");
    }
    request_memory_size = GetRequestMemoryUsage();
    netframe_send_memory_size = GetNetSendMemoryUsage();
    netframe_receive_memory_size = GetNetReceiveMemoryUsage();
    request_count = GetProcessingRequestCount();
    VLOG(2) << "netframe_send_memory_size: " << netframe_send_memory_size
        << ", recv_memory_size: " << netframe_receive_memory_size;
    if (GetMemoryUsedBytes(pid, &vm_size, &mem_size)) {
        dict.SetValue("VM_SIZE", FormatBinaryMeasure(vm_size, " B"));
        dict.SetValue("PM_SIZE", FormatBinaryMeasure(mem_size, " B"));
        dict.SetValue("NF_SEND_SIZE", FormatBinaryMeasure(netframe_send_memory_size, " B"));
        dict.SetValue("NF_RECV_SIZE", FormatBinaryMeasure(netframe_receive_memory_size, " B"));
        dict.SetValue("REQUEST_SIZE", FormatBinaryMeasure(request_memory_size, " B"));
        dict.SetIntValue("REQUEST_COUNT", request_count);
        dict.SetValue("MM_SIZE", FormatBinaryMeasure((request_memory_size +
                                                      netframe_send_memory_size +
                                                      netframe_receive_memory_size), " B"));
        dict.ShowSection("MEMORY_SUCCESS");
    } else {
        dict.ShowSection("MEMORY_ERROR");
    }

    // Get profiler info.
    ProfilerState state;
#ifndef _WIN32
    // disable profiler on windows
    ProfilerGetCurrentState(&state);
    if (state.enabled) {
        dict.SetIntValue("SAMPLES_GATHERED", state.samples_gathered);
        dict.ShowSection("PROFILE");
    }
#endif

    m_stats_registry.Dump(&dict);

    // Fill connections info.
    FillConnectionsInfo(&dict);

    std::string html_template_filename = "status.html";
    StringPiece html_template = STATIC_RESOURCE(poppy_static_status_html);
    ctemplate::StringToTemplateCache(html_template_filename,
        html_template.data(), html_template.size(), ctemplate::STRIP_WHITESPACE);
    std::string output;
    ctemplate::ExpandTemplate(html_template_filename,
        ctemplate::STRIP_WHITESPACE, &dict, &output);

    response->SetHeader("Content-Type", "text/html");
    response->set_body(output);
    done->Run();
}

void RpcServer::FillConnectionsInfo(ctemplate::TemplateDictionary* dict)
{
    std::vector<TrafficStatsItem> traffic_stats_result;
    mutable_traffic_stats()->Dump(&traffic_stats_result);

    dict->SetFormattedValue("CONNECTION_TOTAL_COUNT",
        "%zu", traffic_stats_result.size());
    std::vector<TrafficStatsItem>::const_iterator it;
    for (it = traffic_stats_result.begin();
        it != traffic_stats_result.end(); ++it) {
        ctemplate::TemplateDictionary* row_dict =
            dict->AddSectionDictionary("CONNECTION_ROW");
        row_dict->SetValue("CLIENT_ADDRESS", it->client_address);
        row_dict->SetValue("RX_LAST_SECOND_COUNT",
            FormatMeasure(it->rx_stats_result.last_second_count));
        row_dict->SetValue("RX_LAST_MINUTE_COUNT",
            FormatMeasure(it->rx_stats_result.last_minute_count));
        row_dict->SetValue("RX_LAST_HOUR_COUNT",
            FormatMeasure(it->rx_stats_result.last_hour_count));
        row_dict->SetValue("RX_TOTAL_COUNT",
            FormatMeasure(it->rx_stats_result.total_count));
        row_dict->SetValue("TX_LAST_SECOND_COUNT",
            FormatMeasure(it->tx_stats_result.last_second_count));
        row_dict->SetValue("TX_LAST_MINUTE_COUNT",
            FormatMeasure(it->tx_stats_result.last_minute_count));
        row_dict->SetValue("TX_LAST_HOUR_COUNT",
            FormatMeasure(it->tx_stats_result.last_hour_count));
        row_dict->SetValue("TX_TOTAL_COUNT",
            FormatMeasure(it->tx_stats_result.total_count));
    }
}

void RpcServer::HandleHealthRequest(const HttpRequest* request,
                                    HttpResponse* response,
                                    Closure<void>* done)
{
    response->SetHeader("Content-Type", "text/plain");
    if (IsHealthy())
    {
        response->set_body("OK");
    }
    else
    {
        response->set_body("Unhealthy");
    }
    done->Run();
}

// filters vars are separated by commas, such as cpu_usage,float_number
// wrap it in a separate method in order to deal with more complicated
// situations, such as wildcards.
void RpcServer::ParseFilterVars(const std::string& filter_value,
                                std::vector<std::string>* variables)
{
    SplitString(filter_value, ",", variables);
}

void RpcServer::HandleVarsRequest(const HttpRequest* request,
                                  HttpResponse* response,
                                  Closure<void>* done)
{
    CgiParams params;
    params.ParseFromUrl(request->uri());
    std::string output_format;
    std::string filter_value;
    std::vector<std::string> variables;
    if (!params.GetValue("of", &output_format)) {
        output_format = "text";
    }
    if (params.GetValue("filter", &filter_value)) {
        RpcServer::ParseFilterVars(filter_value, &variables);
    }
    if (output_format != "json")
        output_format = "text";

    int32_t pid = getpid();
    GetCpuUsageSinceLastCall(pid, &m_cpu_usage);
    GetMemoryUsedBytes(pid,
                       &m_virtual_memory_usage,
                       &m_physical_memory_usage);

    std::ostringstream os;
    ExportedVariables::Dump(os, output_format, &variables);
    if (output_format == "json")
        response->SetHeader("Content-Type", "application/json");
    else
        response->SetHeader("Content-Type", "text/plain");
    response->mutable_http_body()->assign(os.str());
    done->Run();
}

void RpcServer::HandleVersionRequest(const HttpRequest* request,
                                     HttpResponse* response,
                                     Closure<void>* done)
{
    std::ostringstream os;

    os << "Program name:\t" << ThisProcess::BinaryPath() << "\n"
       << "Build time:\t" << binary_version::kBuildTime << "\n"
       << "Build name:\t" << binary_version::kBuilderName << "\n"
       << "Host name:\t" << binary_version::kHostName << "\n"
       << "Compiler:\t" << binary_version::kCompiler << "\n"
       << "Svn info:\n";

    for (int i = 0; i < binary_version::kSvnInfoCount; ++i) {
        os << binary_version::kSvnInfo[i];
    }

    response->SetHeader("Content-Type", "text/plain");
    response->mutable_http_body()->assign(os.str());
    done->Run();
}

void RpcServer::HandleIndexRequest(const HttpRequest* request,
                                   HttpResponse* response,
                                   Closure<void>* done)
{
    std::string html_template_filename = "poppy.html";
    StringPiece html_template = STATIC_RESOURCE(poppy_static_poppy_html);
    // TODO(ericliu): load the template once, instead of per request.
    ctemplate::StringToTemplateCache(html_template_filename,
        html_template.data(), html_template.size(), ctemplate::STRIP_WHITESPACE);
    std::string output;
    {
        MutexLocker locker(m_displayed_links_mutex);
        ctemplate::ExpandTemplate(html_template_filename,
            ctemplate::STRIP_WHITESPACE, &m_displayed_links_dict, &output);
    }

    response->SetHeader("Content-Type", "text/html");
    response->SetHeader("Content-Location", kPoppyHttpPath);
    response->set_body(output);
    done->Run();
}

void RpcServer::HandleGetFlagsRequest(const HttpRequest* request,
                                      HttpResponse* response)
{
    std::vector<google::CommandLineFlagInfo> flag_info;
    GetAllFlags(&flag_info);

    ctemplate::TemplateDictionary dict("data");
    std::vector<google::CommandLineFlagInfo>::iterator it = flag_info.begin();
    while (it != flag_info.end()) {
        const std::string& filename = it->filename;
        ctemplate::TemplateDictionary* file_row_dict =
            dict.AddSectionDictionary("FILE_ROW");
        file_row_dict->SetValue("FILE_NAME", filename);
        while ((it != flag_info.end()) && (filename == it->filename)) {
            ctemplate::TemplateDictionary* flag_row_dict =
                file_row_dict->AddSectionDictionary("FLAG_ROW");
            if (!it->is_default) {
                flag_row_dict->SetValue("FLAG_STYLE", "color:red");
            } else {
                flag_row_dict->SetValue("FLAG_STYLE", "");
            }
            flag_row_dict->SetValue("FLAG_NAME", it->name);
            flag_row_dict->SetValue("FLAG_TYPE", it->type);
            flag_row_dict->SetValue("FLAG_CURRENT_VALUE", it->current_value);
            flag_row_dict->SetValue("FLAG_DEFAULT_VALUE", it->default_value);
            flag_row_dict->SetValue("FLAG_DESCRIPTION", it->description);
            ++it;
        }
    }
    std::string html_template_filename = "flags.html";
    StringPiece html_template = STATIC_RESOURCE(poppy_static_flags_html);
    ctemplate::StringToTemplateCache(html_template_filename,
                                     html_template.data(), html_template.size(),
                                     ctemplate::STRIP_WHITESPACE);
    std::string output;
    ctemplate::ExpandTemplate(html_template_filename,
                              ctemplate::STRIP_WHITESPACE, &dict, &output);

    response->SetHeader("Content-Type", "text/html");
    response->set_body(output);
}

void RpcServer::HandleModifyFlagsRequest(const HttpRequest* request,
                                         HttpResponse* response)
{
    std::string body = request->http_body();
    Json::Value root;
    Json::Reader reader;
    bool set_flag = true;
    std::map<std::string, std::string> error_value_map;
    if (!reader.parse(body, root)) {
        LOG(ERROR) << "Parse flag modify json request error: " << body;
        set_flag = false;
    } else {
        if (root.size() > 0) {
            for (Json::ValueIterator iter = root.begin();
                 iter != root.end();
                 ++iter) {
                std::string new_value =
                    google::SetCommandLineOption(iter.key().asString().data(),
                                                 root[iter.key().asString()].asString().data());
                if (new_value.empty()) {
                    error_value_map[iter.key().asString()] = root[iter.key().asString()].asString();
                    set_flag = false;
                }
            }
        }
    }
    std::string output;
    if (set_flag) {
        output = "[]";
    } else {
        output = "{";
        for (std::map<std::string, std::string>::const_iterator iter = error_value_map.begin();
             iter != error_value_map.end();
             ++iter) {
            if (iter != error_value_map.begin()) {
                output += ", \"";
            } else {
                output += "\"";
            }
            output += (*iter).first;
            output += "\" : \"";
            output += (*iter).second;
            output += "\"";
        }
        output += "}";
    }
    response->set_status(200);
    response->SetHeader("Content-Type", "application/json");
    response->set_body(output);
}

void RpcServer::HandleFlagsRequest(const HttpRequest* request,
                                   HttpResponse* response,
                                   Closure<void>* done)
{
    if (request->method() == HttpRequest::METHOD_GET) {
        HandleGetFlagsRequest(request, response);
    } else if (request->method() == HttpRequest::METHOD_POST) {
        HandleModifyFlagsRequest(request, response);
    } else {
        LOG(ERROR) << "Invalid method, " << request->method();
        std::string output = "error method";
        response->set_status(400);
        response->SetHeader("Content-Type", "text/html");
        response->set_body(output);
    }
    done->Run();
}

void RpcServer::ProfileComplete(
    const char* filename,
    const HttpRequest* request,
    HttpResponse* response,
    Closure<void>* done,
    uint64_t timer_id)
{
#ifndef _WIN32
    // disable profiler on windows
    ProfilerStop();
    ProfilerFlush();
#endif
    io::textfile::LoadToString(filename, response->mutable_http_body());
    response->SetHeader("Content-Type", "application/octet-stream");
    unlink(filename);
    AtomicSet(&m_is_profiling, false);
    done->Run();
}

void RpcServer::HandleProfileRequest(const HttpRequest* request,
                                     HttpResponse* response,
                                     Closure<void>* done)
{
    if (AtomicExchange(&m_is_profiling, true)) {
        LOG(INFO) << "There is another user is profiling. quit.";
        response->SetHeader("Content-Type", "text/plain");
        response->set_body("There is another user is profiling.");
        done->Run();
        return;
    }

    LOG(INFO) << "Start poppy profiling...";

    int seconds = 30;
    CgiParams parms;
    parms.ParseFromUrl(request->uri());
    parms.GetValue("seconds", &seconds);

    const char* filename = "profiler.dat";
#ifndef _WIN32
    // disable profiler on windows
    ProfilerStart(filename);
#endif
    Closure<void, uint64_t>* closure =
        NewClosure(this, &RpcServer::ProfileComplete, filename, request, response, done);
    m_timer_manager.AddOneshotTimer(seconds * 1000, closure);
}

void RpcServer::RegisterHandler(const std::string& path, HttpHandler* handler)
{
    CHECK(path != kRpcHttpPath);
    HttpServer::RegisterHandler(path, handler);
}

void RpcServer::AddLinkOnIndexPage(const std::string& path, const std::string& display_name)
{
    MutexLocker locker(m_displayed_links_mutex);
    ctemplate::TemplateDictionary* link_dict =
        m_displayed_links_dict.AddSectionDictionary("LINK");
    link_dict->SetValue("URL", path.substr(1));
    link_dict->SetValue("ANCHOR", display_name);
}

void RpcServer::RegisterService(google::protobuf::Service* service)
{
    HttpHandler* handler = FindHandler(kRpcHttpPath);
    if (handler == NULL) {
        return;
    }
    DCHECK_NOTNULL(dynamic_cast<RpcServerHandler*>(handler)); // NOLINT(runtime/rtti)

    // Register methods
    int method_num = service->GetDescriptor()->method_count();
    for (int i = 0; i < method_num; ++i) {
        m_stats_registry.RegisterMethod(service->GetDescriptor()->method(i)->full_name());
    }
    static_cast<RpcServerHandler*>(handler)->RegisterService(service);
}

bool RpcServer::CallServiceMethod(google::protobuf::Service* service,
                                  const google::protobuf::MethodDescriptor* method,
                                  RpcController* controller,
                                  const google::protobuf::Message* request,
                                  google::protobuf::Message* response,
                                  google::protobuf::Closure* done)
{
    DCHECK(controller != NULL) << "controller should not be NULL";

    if (AtomicGet(&m_has_stopped)) {
        VLOG(4) << "RpcServer has already been stopped";
        return false;
    }

    RegisterRequest(controller);

    bool success;
    {
        RWLock::ReaderLocker locker(m_stop_rwlock);
        success = !AtomicGet(&m_has_stopped);
        if (success) {
            service->CallMethod(method, controller, request, response, done);
        } else {
            VLOG(4) << "RpcServer has already been stopped";
        }
    }

    if (!success)
        UnregisterRequest(controller);

    return success;
}

void RpcServer::RegisterRequest(RpcController* controller)
{
    m_processing_requests.RegisterRequest(controller);
}

void RpcServer::UnregisterRequest(RpcController* controller)
{
    m_processing_requests.UnregisterRequest(controller);
}

size_t RpcServer::GetRequestMemoryUsage()
{
    return AtomicGet(&m_request_memory_size);
}

int64_t RpcServer::GetProcessingRequestCount() const
{
    return m_processing_requests.Count();
}

size_t RpcServer::GetNetSendMemoryUsage()
{
    size_t memory_size = mutable_net_frame()->GetCurrentSendBufferedLength();
    m_stats_registry.AddNetFrameSendMemorySize(memory_size,
                                               GetTimeStampInMs() / 1000);
    return memory_size;
}

size_t RpcServer::GetNetReceiveMemoryUsage()
{
    size_t memory_size = mutable_net_frame()->GetCurrentReceiveBufferedLength();
    m_stats_registry.AddNetFrameReceiveMemorySize(memory_size,
                                                  GetTimeStampInMs() / 1000);
    return memory_size;
}

const RpcStatsInfo& RpcServer::GetGlobalRpcStats() const
{
    return m_stats_registry.GetGlobalRpcStats();
}

void RpcServer::IncreaseRequestMemorySize(size_t memory_size)
{
    size_t request_memory_size = AtomicAdd(&m_request_memory_size, memory_size);
    m_stats_registry.UpdateRpcMemorySize(request_memory_size,
                                         GetTimeStampInMs() / 1000);
}

void RpcServer::DecreaseRequestMemorySize(size_t memory_size)
{
    size_t request_memory_size = AtomicSub(&m_request_memory_size, memory_size);
    m_stats_registry.UpdateRpcMemorySize(request_memory_size,
                                         GetTimeStampInMs() / 1000);
}

void RpcServer::RegisterBuiltinHandlers()
{
    m_rpc_handler = new RpcServerHandler(this);
    HttpServer::RegisterHandler(kRpcHttpPath, m_rpc_handler);

    HttpServer::RegisterSimpleHandler(kStatusHttpPath,
            NewPermanentClosure(this, &RpcServer::HandleStatusRequest));

    HttpServer::RegisterSimpleHandler(kHealthHttpPath,
            NewPermanentClosure(this, &RpcServer::HandleHealthRequest));

    HttpServer::RegisterSimpleHandler(kVarsHttpPath,
            NewPermanentClosure(this, &RpcServer::HandleVarsRequest));

    HttpServer::RegisterSimpleHandler(kVersionHttpPath,
            NewPermanentClosure(this, &RpcServer::HandleVersionRequest));

    HttpServer::RegisterSimpleHandler(kProfileHttpPath,
            NewPermanentClosure(this, &RpcServer::HandleProfileRequest));

    HttpServer::RegisterSimpleHandler(kFlagsHttpPath,
            NewPermanentClosure(this, &RpcServer::HandleFlagsRequest));

    HttpServer::RegisterPrefixSimpleHandler(kRpcFormHttpPath,
            NewPermanentClosure(HandleFormRequest));

    HttpServer::RegisterPrefixSimpleHandler(kRpcServiceHttpPath,
            NewPermanentClosure<void, const RpcServerHandler*>(
                    HandleServiceRequest, m_rpc_handler));

    RegisterStaticResources();

    // register /poppy and / path
    HttpServer::RegisterSimpleHandler(kPoppyHttpPath,
            NewPermanentClosure(this, &RpcServer::HandleIndexRequest));
    HttpServer::RegisterSimpleHandler("/",
            NewPermanentClosure(this, &RpcServer::HandleIndexRequest));

    // Add links on index page.
    AddLinkOnIndexPage(kStatusHttpPath, "status");
    AddLinkOnIndexPage(kVersionHttpPath, "binary version");
    AddLinkOnIndexPage(kHealthHttpPath, "health");
    AddLinkOnIndexPage(kVarsHttpPath, "variables");
    AddLinkOnIndexPage(kFlagsHttpPath, "flags info");
    AddLinkOnIndexPage(kRpcFormHttpPath, "rpc service forms");
}

void RpcServer::RegisterBuiltinServices()
{
    RegisterService(new RpcBuiltinService(this, m_rpc_handler));
}

void RpcServer::UnregisterAllServices()
{
    HttpHandler* handler = FindHandler(kRpcHttpPath);
    if (handler == NULL) {
        return;
    }

    DCHECK_NOTNULL(dynamic_cast<RpcServerHandler*>(handler)); // NOLINT(runtime/rtti)
    static_cast<RpcServerHandler*>(handler)->UnregisterAllServices();
}

} // namespace poppy
