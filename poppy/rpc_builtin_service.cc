// Copyright (c) 2011, Tencent Inc.
// All rights reserved.
//
// Author: Dongping HUANG <dphuang@tencent.com>
//         Yongsong Liu <ericliu@tencent.com>
// Created: 08/18/11
// Description:

#include "poppy/rpc_builtin_service.h"
#include <map>
#include <set>
#include <string>
#include <vector>
#include "common/base/string/string_number.h"
#include "poppy/rpc_handler.h"
#include "poppy/rpc_server.h"
#include "thirdparty/protobuf/descriptor.h"

namespace poppy {

// using namespace common;

namespace {

void FillEnumType(MethodInputTypeDescriptorsResponse* response,
                  const google::protobuf::EnumDescriptor* enum_descriptor)
{
    poppy::EnumDescriptor* enum_type = response->add_enum_types();
    enum_type->set_name(enum_descriptor->full_name());
    for (int index = 0; index < enum_descriptor->value_count(); index++) {
        const google::protobuf::EnumValueDescriptor *
                enum_value_descriptor = enum_descriptor->value(index);
        poppy::EnumValueDescriptor* enum_type_item = enum_type->add_values();
        enum_type_item->set_name(enum_value_descriptor->name());
        enum_type_item->set_number(enum_value_descriptor->number());
    }
}

void GetAllFieldDescriptors(const google::protobuf::Descriptor* descriptor,
                            std::vector<const google::protobuf::FieldDescriptor*>& fields)
{
    google::protobuf::MessageFactory * message_factory =
            google::protobuf::MessageFactory::generated_factory();
    const google::protobuf::Message* message = message_factory->GetPrototype(descriptor);
    const google::protobuf::Reflection* reflection = message->GetReflection();
    for (int i = 0; i < descriptor->extension_range_count(); i++) {
        const google::protobuf::Descriptor::ExtensionRange* ext_range =
                descriptor->extension_range(i);
        for (int tag_number = ext_range->start; tag_number < ext_range->end; tag_number++) {
            const google::protobuf::FieldDescriptor* field =
                    reflection->FindKnownExtensionByNumber(tag_number);
            if (!field) continue;
            fields.push_back(field);
        }
    }
    for (int i = 0; i < descriptor->field_count(); i++) {
        fields.push_back(descriptor->field(i));
    }
}

void FillMessageType(MethodInputTypeDescriptorsResponse* response,
                     std::set<std::string>* added_type_set,
                     const google::protobuf::Descriptor* message_descriptor)
{
    static const int kCppTypeBytes = 11;
    std::vector<const google::protobuf::FieldDescriptor*> fields;
    GetAllFieldDescriptors(message_descriptor, fields);

    poppy::MessageDescriptor* message_type = response->add_message_types();
    message_type->set_name(message_descriptor->full_name());
    for (size_t i = 0; i < fields.size(); i++) {
        const google::protobuf::FieldDescriptor* field = fields[i];
        poppy::FieldDescriptor* field_type = message_type->add_fields();
        field_type->set_name(field->name());
        field_type->set_number(field->number());
        field_type->set_label(field->label());
        if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES) {
            field_type->set_type(kCppTypeBytes);
        } else {
            field_type->set_type(field->cpp_type());
        }
        if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_ENUM) {
            const google::protobuf::EnumDescriptor* enum_type = field->enum_type();
            field_type->set_type_name(enum_type->full_name());
            if (added_type_set->insert(enum_type->full_name()).second)
                FillEnumType(response, enum_type);
        } else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
            const google::protobuf::Descriptor* message_type = field->message_type();
            field_type->set_type_name(message_type->full_name());
            if (added_type_set->insert(message_type->full_name()).second)
                FillMessageType(response, added_type_set, message_type);
        }

        if (field->has_default_value()) {
            switch (field->cpp_type()) {
#define CASE_FIELD_TYPE(cpptype, suffix)                                    \
                case google::protobuf::FieldDescriptor::CPPTYPE_##cpptype:  \
                    field_type->set_default_value(                          \
                        NumberToString(field->default_value_##suffix()));   \
                    break;                                                  \

                CASE_FIELD_TYPE(INT32,  int32);
                CASE_FIELD_TYPE(UINT32, uint32);
                CASE_FIELD_TYPE(INT64,  int64);
                CASE_FIELD_TYPE(UINT64, uint64);
                CASE_FIELD_TYPE(FLOAT,  float);
                CASE_FIELD_TYPE(DOUBLE, double);
#undef CASE_FIELD_TYPE

                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                    field_type->set_default_value(NumberToString(
                            static_cast<int>(field->default_value_bool())));
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                    field_type->set_default_value(NumberToString(
                            field->default_value_enum()->number()));
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                    field_type->set_default_value(field->default_value_string());
                    break;

                default:
                    break;
            }
        }
    }
}

void FillAllTypesOfRequest(
    const google::protobuf::MethodDescriptor* method,
    MethodInputTypeDescriptorsResponse* response)
{
    const google::protobuf::Descriptor* descriptor = method->input_type();
    std::vector<const google::protobuf::FieldDescriptor*> fields;
    GetAllFieldDescriptors(descriptor, fields);

    response->set_request_type(descriptor->full_name());
    std::set<std::string> added_type_set;
    FillMessageType(response, &added_type_set, descriptor);
    added_type_set.insert(descriptor->full_name());

    for (size_t i = 0; i < fields.size(); i++) {
        const google::protobuf::FieldDescriptor* field = fields[i];
        switch (field->cpp_type()) {
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                {
                    const google::protobuf::EnumDescriptor* enum_type = field->enum_type();
                    if (added_type_set.insert(enum_type->full_name()).second)
                        FillEnumType(response, enum_type);
                }
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                {
                    const google::protobuf::Descriptor* message_type = field->message_type();
                    if (added_type_set.insert(message_type->full_name()).second)
                        FillMessageType(response, &added_type_set, message_type);
                }
                break;

            default:
                break;
        }
    }
}

} // namespace

RpcBuiltinService::RpcBuiltinService(RpcServer* rpc_server,
                                     RpcServerHandler* rpc_handler)
    : m_rpc_server(rpc_server), m_rpc_handler(rpc_handler)
{
}

void RpcBuiltinService::Health(google::protobuf::RpcController* controller,
                               const poppy::EmptyRequest* request,
                               HealthResponse* response,
                               google::protobuf::Closure* done)
{
    if (m_rpc_server && m_rpc_server->IsHealthy()) {
        response->set_health("OK");
    } else {
        response->set_health("");
    }

    done->Run();
}

void RpcBuiltinService::GetAllServiceDescriptors(
    google::protobuf::RpcController* controller,
    const poppy::EmptyRequest* request,
    AllServiceDescriptors* response,
    google::protobuf::Closure* done)
{
    const std::map<std::string, google::protobuf::Service*>& services =
        m_rpc_handler->GetAllServices();
    for (std::map<std::string, google::protobuf::Service*>::const_iterator
            it = services.begin(); it != services.end(); ++it) {
        // skip builtin service
        if (it->first == BuiltinService::descriptor()->full_name())
            continue;

        const google::protobuf::ServiceDescriptor*
                service_descriptor = it->second->GetDescriptor();

        poppy::ServiceDescriptor* service =
                response->add_services();
        service->set_name(service_descriptor->full_name());
        for (int i = 0; i < service_descriptor->method_count(); i++) {
            poppy::MethodDescriptor* method = service->add_methods();
            method->set_name(service_descriptor->method(i)->name());
        }
    }
    done->Run();
}

void RpcBuiltinService::GetMethodInputTypeDescriptors(
    google::protobuf::RpcController* controller,
    const poppy::MethodInputTypeDescriptorsRequest* request,
    MethodInputTypeDescriptorsResponse* response,
    google::protobuf::Closure* done)
{
    RpcController* rpc_controller =
        static_cast<RpcController*>(controller);
    std::string service_name;
    std::string method_name;
    if (!ParseMethodFullName(request->method_full_name(),
            &service_name, &method_name)) {
        LOG(ERROR) << "Method full name " << request->method_full_name() << " is invalid";
        rpc_controller->SetFailed(RPC_ERROR_METHOD_NAME, request->method_full_name());
        done->Run();
        return;
    }

    google::protobuf::Service* service = m_rpc_handler->FindServiceByName(service_name);
    if (service == NULL) {
        LOG(ERROR) << "Service " << service_name
            << " not found, method full name is " << request->method_full_name();
        rpc_controller->SetFailed(RPC_ERROR_FOUND_SERVICE, request->method_full_name());
        done->Run();
        return;
    }

    const google::protobuf::MethodDescriptor* method =
            service->GetDescriptor()->FindMethodByName(method_name);
    if (method == NULL) {
        LOG(ERROR) << "Method " << method_name << " not found, method full name is "
            << request->method_full_name();
        rpc_controller->SetFailed(RPC_ERROR_FOUND_METHOD, request->method_full_name());
        done->Run();
        return;
    }

#if defined(LIST_TODO)
#pragma message("\nTODO(ericliu) 由MethodDescriptor获取相关类型信息填充" \
                "MethodInputTypeDescriptorsResponse结构\n" \
                "MethodDescriptor所描述的方法在第一次被用户通过form" \
                "调用的时候执行真正的填充操作，执行结果可以通过全局" \
                "静态变量缓存，后续的任何用户再次通过form调用该方法" \
                "的时候只需读取该缓存即可，注意，要保证多线程安全")
#endif

    FillAllTypesOfRequest(method, response);
    done->Run();
}

} // namespace poppy
