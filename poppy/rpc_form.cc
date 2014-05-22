// Copyright 2011, Tencent Inc.
// Author: Eric Liu <ericliu@tencent.com>

#include "poppy/rpc_form.h"

#include <string>
#include <vector>
#include "common/base/static_resource.h"
#include "common/net/http/http_message.h"
#include "common/system/net/ip_address.h"

#include "poppy/rpc_builtin_service.h"
#include "poppy/rpc_handler.h"
#include "poppy/rpc_server.h"
#include "poppy/static_resource.h"

#include "thirdparty/ctemplate/template.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/protobuf/descriptor.h"

namespace poppy {

// using namespace common;

void HandleFormRequest(const HttpRequest* http_request,
                       HttpResponse* http_response,
                       Closure<void>* done)
{
    if (http_request->method() != HttpRequest::METHOD_GET) {
        http_response->set_status(405);
        http_response->set_body("Only accept get method.");
        done->Run();
        return;
    }

    std::string host_name;
    http_request->GetHeader("Host", &host_name);

    std::string service_name;
    std::string method_name;
    bool ret = ParseMethodFullNameFromPath(http_request->uri(),
            kRpcFormHttpPath, &service_name, &method_name);
    if (!ret || (service_name.empty() && !method_name.empty()) ||
            (!service_name.empty() && method_name.empty())) {
        LOG(ERROR) << "Method name is invalid, request path is " << http_request->uri();
        http_response->set_status(400);
        http_response->set_body("Method name is invalid");
        done->Run();
        return;
    }

    std::string html_template_filename;
    StringPiece html_template;
    ctemplate::TemplateDictionary dict("data");
    dict.SetValue("REGISTRY_NAME",
            BuiltinService::descriptor()->full_name());
    if (service_name.empty() && method_name.empty()) {
        html_template_filename = "forms.html";
        dict.SetValue("HOST_NAME", host_name);
        html_template = STATIC_RESOURCE(poppy_static_forms_html);
    } else {
        html_template_filename = "methods.html";
        dict.SetValue("SERVICE_NAME", service_name);
        dict.SetValue("METHOD_NAME", method_name);
        html_template = STATIC_RESOURCE(poppy_static_methods_html);
    }

    ctemplate::StringToTemplateCache(html_template_filename,
        html_template.data(), html_template.size(), ctemplate::STRIP_WHITESPACE);
    std::string output;
    ctemplate::ExpandTemplate(html_template_filename,
            ctemplate::STRIP_WHITESPACE, &dict, &output);

    http_response->SetHeader("Content-Type", "text/html; charset=utf-8");
    http_response->set_body(output);
    done->Run();
}

} // namespace poppy
