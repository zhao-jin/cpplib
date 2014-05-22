// Copyright (C) 2011 - Tencent Inc.
// Author: Xiang Wen <flyingwen@tencent.com>
//         Yi Wang <yiwang@tencent.com>
//
// Print records in binary data files into human-readable format.
//
//
// Example: Dump RecordIO file.
//
// Each record in RecordIO files can be almost any data.  Consider
// RecordIO files, ~/wordcount/output/log*.recordio, whose each record
// is the serialization of a protocol message TestRecord defined in
// ~/src/test_record.proto and encapsulated in package learn_codex.
// We can dump the content using:
/*
   codex --recordio="%P{learn_codex.TestRecord}"        \
     --protofiles="~/src/test_record.proto"             \
     ~/wordcount/output/log*.recordio
*/
// Note that the conversion %P denotes a protocol buffer message,
// followed by the package name and message name, separated by '.' and
// enclosed by '{}'.  Other conversion include %d for integer, %c for
// character and %s for NULL-terminated string.  For a complete
// reference to all conversions, please refer to the head comment in
// format_string_parser.h.
//
//
// Example: Dump SSTable file.
//
// Each record in SSTable files consists of a key and a value.
// Consider an SSTable file, ./test.sstable, whose key is a 64-bit
// little-endian integer, and whose value is the serialization of
// protocol message TestProtoMessage defined in ./codex_test.proto.
// We can dump its content using:
/*
   codex --sstable='key=%lld %s :value=%P{codex.TestProtoMessage}'  \
     --protofiles=./codex_test.proto                                \
     test.sstable
*/
// Note that the special character ':' is used to separate formats of
// key and value.  You can try the command yourself, and see what the
// result looks like
//
//
// Example: Support #import in .proto Files
//
// When we develop in a source code tree, say rooted as ~/work, it is
// often that a .proto file imports many other .proto files
// (recursively).  It is impractical to require one to specify all
// these .proto files on the command line of codex.  Instead, codex is
// smart that you only need to specify the .proto file whose contains
// the message definition used in your data file, and codex will find
// others, as long as we tell codex the source tree root.
//
// For example, consider the source tree root is ~/work, and
// ~/work/system1/a.proto imports ~/work/system2/b.proto by
//   #import "system2/b.proto"
//
// The codex command in above example becomes:
/*
   codex --sstable='key=%lld %s :value=%P{codex.TestProtoMessage}' \
     --proto_path=~/work                                           \
     --protofiles=~/work/system1/codex_test.proto                  \
     test.sstable
*/

#include <iostream>
#include <string>
#include <vector>

#include "common/base/scoped_ptr.h"
#include "common/base/singleton.h"
#include "common/base/string/algorithm.h"
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"

#include "common/tools/codex/field_parser.h"
#include "common/tools/codex/file_reader.h"
#include "common/tools/codex/format_string_parser.h"
#include "common/tools/codex/proto_message_creator.h"
#include "common/tools/codex/recordio_reader.h"
#include "common/tools/codex/sstable_reader.h"

DEFINE_string(raw, "", "format in raw file");
DEFINE_string(recordio, "", "format in recordio file");
DEFINE_string(sstable, "", "format in sstable file");
DEFINE_string(protofiles, "", ".proto source code");
DEFINE_string(proto_path, ".",
              "Specify the directory in which to search for "
              "imports.  Directories are separated by colon, "
              "and will be searched in order.  If not "
              "given, the current working directory is used.");
DEFINE_string(key, "",
              "print particular record(s) in file. "
              "for sstable, it prints the record with specified key. "
              "for recordio, it prints the [key]th value in file.");
DEFINE_string(begin, "",
              "set the range of printed records, used with --end. "
              "for sstable, it prints all records whose key btween [begin] and [end]. "
              "for recordio, it prints from the [begin]th record to the [end]th record.");
DEFINE_string(end, "", "use with --begin");

// namespace common {
namespace codex {

#if defined(_WIN32) && !defined(__CYGWIN__)
static const char* kPathSeparator = ";";
#else
static const char* kPathSeparator = ":";
#endif

// Parsing import paths specified in FLAGS_proto_path, and
// add these pathes to creator.
void SetupImportPaths(ProtoMessageCreator* creator) {
    std::vector<std::string> parts;
    SplitString(FLAGS_proto_path, kPathSeparator, &parts);

    bool all_paths_accessible = true;
    for (uint i = 0; i < parts.size(); ++i) {
        if (access(parts[i].c_str(), F_OK) < 0) {
            LOG(ERROR) << parts[i] << " does not exist or is not accessible.";
            all_paths_accessible = false;
        }
        creator->AddImportPath(parts[i]);
    }

    if (!all_paths_accessible) {
        LOG(FATAL) << "Not all proto_paths exist and accessible.";
    }
}

// register proto source files
void LoadProtoFiles(ProtoMessageCreator* creator) {
    std::vector<std::string> parts;
    SplitString(FLAGS_protofiles, kPathSeparator, &parts);

    bool all_files_correct = true;
    for (uint i = 0; i < parts.size(); ++i) {
        if (!creator->AddProtoSourceFile(parts[i])) {
            all_files_correct = false;
        }
    }

    if (!all_files_correct) {
        LOG(FATAL) << "Error in compiling .proto files:\n"
                   << creator->GetError();
    }
}

void CreateRecordParsers(std::string *file_type,
                         std::vector<FieldParserSequence*> *parsers_list) {
    const char* kFieldDelim = ":";

    int count = 0;
    const std::string* format_string = NULL;
    if (!FLAGS_raw.empty()) {
        *file_type = "raw";
        format_string = &FLAGS_raw;
        ++count;
    }
    if (!FLAGS_recordio.empty()) {
        *file_type = "recordio";
        format_string = &FLAGS_recordio;
        ++count;
    }
    if (!FLAGS_sstable.empty()) {
        *file_type = "sstable";
        format_string = &FLAGS_sstable;
        ++count;
    }
    CHECK_EQ(count, 1)
        << "please use --recordio or --sstable to specify format";

    // parse format string
    std::vector<std::string> sub_strings;
    SplitString(*format_string, kFieldDelim, &sub_strings);
    for (size_t i = 0; i < sub_strings.size(); ++i) {
        FieldFormatSequence formats;
        ParseFieldFormats(sub_strings[i], &formats);
        FieldParserSequence *parsers = new FieldParserSequence(formats);
        parsers_list->push_back(parsers);
    }
}

void PrintOneRecord(const std::vector<FieldParserSequence*> &parsers_list,
                    const std::vector<StringPiece> &fields) {
    CHECK_EQ(parsers_list.size(), fields.size());
    for (size_t i = 0; i < parsers_list.size(); ++i) {
        FieldParserSequence *parsers = parsers_list[i];
        parsers->ParseAndPrintField(fields[i], &std::cout);
    }
    std::cout << std::endl;
}

void PrintFileRecords(const std::vector<FieldParserSequence*> &parsers_list,
                      FileReader *reader) {
    std::vector<StringPiece> fields;
    while (reader->Read(&fields)) {
        PrintOneRecord(parsers_list, fields);
    }
}

// release all resources
void DestoryRecordParsers(std::vector<FieldParserSequence*> *parsers_list) {
    for (size_t i = 0; i < parsers_list->size(); ++i) {
        delete parsers_list->at(i);
    }
    parsers_list->clear();
}

}  // namespace codex
// } // namespace common

// using namespace common;
int main(int argc, char **argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    codex::ProtoMessageCreator &creator =
        Singleton<codex::ProtoMessageCreator>::Instance();
    codex::SetupImportPaths(&creator);
    codex::LoadProtoFiles(&creator);

    std::string file_type;
    std::vector<codex::FieldParserSequence*> parsers_list;
    codex::CreateRecordParsers(&file_type, &parsers_list);

    scoped_ptr<codex::FileReader> reader;
    reader.reset(codex::CreateFileReaderObject(file_type.c_str()));
    CHECK(reader != NULL);
    CHECK(FLAGS_key.empty() || (FLAGS_begin.empty() && FLAGS_end.empty()));
    for (int i = 1; i < argc; ++i) {
        CHECK_EQ(reader->Open(argv[i]), true)
            << "Fail to open file: " << argv[i];

        if (!FLAGS_key.empty()) {
            reader->Seek(FLAGS_key);
        }
        if (!FLAGS_begin.empty() && !FLAGS_end.empty()) {
            reader->Seek(FLAGS_begin, FLAGS_end);
        }
        codex::PrintFileRecords(parsers_list, reader.get());

        reader->Close();
    }

    codex::DestoryRecordParsers(&parsers_list);

    return 0;
}

