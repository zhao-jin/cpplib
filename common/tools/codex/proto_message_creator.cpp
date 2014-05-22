// Copyright 2010 Tencent Inc.
// Author: Yi Wang <yiwang@tencent.com>
//         Xiang Wen <flyingwen@tencent.com>

#include "common/tools/codex/proto_message_creator.h"

#include <stdio.h>
#include <unistd.h>                     // for get_current_dir_name()

#include <string>
#include <vector>

#include "common/base/string/algorithm.h"
#include "thirdparty/glog/logging.h"

// namespace common {
namespace codex {

using std::string;
using std::vector;

ProtoMessageCreator::ProtoMessageCreator()
    : m_error_collector(ErrorPrinter::ERROR_FORMAT_GCC),
      m_source_tree(),
      m_compiler(&m_source_tree, &m_error_collector),
      m_factory(m_compiler.pool()) {}


ProtoMessageCreator::~ProtoMessageCreator() {}


void ProtoMessageCreator::AddImportPath(const string& import_path) {
    m_source_tree.MapPath("", import_path.c_str());
}


bool ProtoMessageCreator::AddProtoSourceFile(const string& proto_filename) {
    std::string relative_path;
    if (MakeInputsBeProtoPathRelative(proto_filename, &relative_path)) {
        if (m_compiler.Import(relative_path) != NULL) {
            return true;
        } else {
            LOG(ERROR) << "Failed compiling proto file: " << proto_filename
                       << " as " << relative_path;
            return false;
        }
    }
    return false;
}


google::protobuf::Message* ProtoMessageCreator::CreateProtoMessage(
    const string& message_name) {
    const google::protobuf::Descriptor* message_desc =
        m_compiler.pool()->FindMessageTypeByName(message_name);
    if (message_desc == NULL) {
        LOG(ERROR) << "Cannot find symbol: " << message_name.c_str();
        return NULL;
    }

    const google::protobuf::Message* prototype_msg =
        m_factory.GetPrototype(message_desc);
    if (prototype_msg == NULL) {
        LOG(ERROR) << "Cannot create prototype message: " <<  message_name;
        return NULL;
    }

    return prototype_msg->New();
}


bool ProtoMessageCreator::MakeInputsBeProtoPathRelative(
    const std::string& input_path,
    std::string* relative_path) {
    using google::protobuf::compiler::DiskSourceTree;

    string shadowing_disk_file;
    switch (m_source_tree.DiskFileToVirtualFile(
            input_path, relative_path, &shadowing_disk_file)) {
        case DiskSourceTree::SUCCESS:
            break;
        case DiskSourceTree::SHADOWED:
            LOG(ERROR) << input_path << ": Input is shadowed in the --proto_path "
                "by \"" << shadowing_disk_file << "\".  Either use the latter "
                "file as your input or reorder invocations to AddImportPath so "
                "that the former file's location comes first.";
            return false;
        case DiskSourceTree::CANNOT_OPEN:
            LOG(ERROR) << input_path << ": " << strerror(errno);
            return false;
        case DiskSourceTree::NO_MAPPING:
            // First check if the file exists at all.
            if (access(input_path.c_str(), F_OK) < 0) {
                // File does not even exist.
                LOG(ERROR) << input_path << ": " << strerror(ENOENT);
            } else {
                LOG(ERROR) << input_path << ": File does not reside within any "
                    "path specified using --proto_path (or -I).  You must "
                    "specify a --proto_path which encompasses this file.  Note "
                    "that the proto_path must be an exact prefix of the .proto "
                    "file names -- protoc is too dumb to figure out when two "
                    "paths (e.g. absolute and relative) are equivalent (it's "
                    "harder than you think).";
            }
            return false;
    }
    return true;
}

}  // namespace codex
// } // namespace common
