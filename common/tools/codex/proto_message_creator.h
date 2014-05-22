// Copyright 2010 Tencent Inc.
// Author: Yi Wang <yiwang@tencent.com>
//
// Support compilation of given proto files and create message
// instances defined in these proto files.  NO NEED for the user to
// give imported proto files, the class tries to find and compile them
// automatically.
//
// Example:
/*
  codex::ProtoMessageCreator creator;
  CHECK(creator.AddImportPath(source_root_dir));
  CHECK(creator.AddProtoSourceFile(proto_file));
  google::protobuf::Message* msg = creator.CreateProtoMessage("KeyValuePB");

  FieldFormat* format = dynamic_cast<FieldFormat*>(msg);
  format->SerializeFromString(...);
  delete format;
*/

#ifndef COMMON_TOOLS_CODEX_PROTO_MESSAGE_CREATOR_H_
#define COMMON_TOOLS_CODEX_PROTO_MESSAGE_CREATOR_H_

#include <string>
#include <vector>

#include "common/tools/codex/error_printer.h"
#include "protobuf/compiler/importer.h"
#include "protobuf/descriptor.h"
#include "protobuf/dynamic_message.h"
#include "protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
class DescriptorPool;
class DynamicMessageFactory;
class FileDescriptor;
class Message;
namespace compiler {
class DiskSourceTree;
class Importer;
} // namespace compiler
} // namespace protobuf
} // namespace google

// namespace common {
namespace codex {

// This class depends on details in google::protobuf::compile.  Here
// summarize these details.  In Google Protobuf, before we can create
// an instance of a protocol message, we need to parse the .proto
// files used to define this message.  In the simple case that only
// one .proto file is involved (i.e., the .proto file does not
// "import" any other .proto files), this procedure is as follows:
//
// .proto file --> FileDescriptorProto --> FileDescriptor --> Descriptor
// (the input)     (parsed content of      (.proto file       (message in the
//                  the .proto file)        descriptor)        .proto file)
//
// Given the Descriptor (of a message), we can use DynamicMessageFactory
// to create a prototype instance of the message, which is immutable.  To
// get an mutable version, we invoke Message::New() using the prototype.
//
// However, the difficult case is that the .proto file imports other
// files, and the message definition depends on messages defined in
// the imported files.  In this case, we need to assemble multiple
// FileDescriptorProto into one FileDescriptor, and use it to get the
// Descriptor of the "assembled" message definiton.
//
// This ``assembling'' is achieved by the following procedure (where | denotes
// function invocation):
//
// Importer::Import(filename)
//  |
// DescriptorPool::FindFileByName(filename)
//  |
// DescriptorPool::TryFindFileInFallbackDatabase(filename)
//  |
// DescriptorDatabase::FindFileByName(filename, &file_proto)
// DescriptorPool::BuildFileFromDatabase(const FileDescriptorProto& proto)
//  |
// DescriptorBuilder::BuildFile(const FileDescriptorProto& proto)
//   for each proto.dependency in proto, recursively invokes
//     DescriptorPool::TryFindFileInFallbackDatabase(filename)
//
// We can see that the assembling is done by DescriptorBuilder::BuildFile,
// which recursively processes imported files using
// DescriptorDatabase::FindFileByName(filename, &file_proto), which opens
// and parses the imported file.
//
// Here the question becomes, before it can
// open the imported file, it must be able to locate the file.  This can be
// done by deriving a class from DescriptorDatabase, which knows how to locate
// imported files.  Fortunately, there has been such a class:
// SourceTreeDescriptorDatabase.
//
// SourceTreeDescriptorDatabase maintains a source tree as a list of
// branches.  In other words, it maintains a set of ``search paths'',
// instead of a tree, and happens meet our requirements:
//
// In the proto file compiler tools (protoc), users are allowed to specify
// an ``import path'', in which and all .proto files resides.  In our case,
// this can be done via ProtoMessageCreator::AddImportPath(...). In addition,
// ProtoMessageCreator::AddProtoSourceFile will add the directory of each
// proto file into ``import paths'' before parsing the file.
//
class ProtoMessageCreator {
public:
    ProtoMessageCreator();
    ~ProtoMessageCreator();

    // AddProtoSourceFile(s) will try to find import files in ``import paths''.
    void AddImportPath(const std::string& import_path);

    // Parse .proto file, while trying to find imported files in
    // ``import paths'' and parse them automatically.
    bool AddProtoSourceFile(const std::string& proto_file);

    // Given a message name, find out the parsed proto file, in which
    // the message is defined; and create corresponding message
    // instance.  If the message is in a proto file which depends on
    // other proto files, assemble these proto files before create the
    // message.
    google::protobuf::Message* CreateProtoMessage(
        const std::string& message_name);

    const std::string& GetError() const {
        return m_error_collector.GetError();
    }

private:
    bool MakeInputsBeProtoPathRelative(const std::string& input_path,
                                       std::string* relative_path);

private:
    ErrorPrinter m_error_collector;
    google::protobuf::compiler::DiskSourceTree m_source_tree;
    google::protobuf::compiler::Importer m_compiler;
    google::protobuf::DynamicMessageFactory m_factory;
};

}  // namespace codex
// } // namespace common

#endif  // COMMON_TOOLS_CODEX_PROTO_MESSAGE_CREATOR_H_
