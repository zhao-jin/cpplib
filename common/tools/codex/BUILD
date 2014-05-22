# Copyright 2011 Tencent Inc.
# Author: Yi Wang <yiwang@tencent.com>
#         Xiang Wen <flyingwen@tencent.com>

proto_library(
    name = 'codex_proto',
    srcs = ['field_formats.proto']
)

proto_library(
    name = 'codex_test_proto',
    srcs = 'codex_test.proto'
)

cc_library(
    name = 'format_string_parser',
    srcs = 'format_string_parser.cpp',
    deps = [':codex_proto',
            '//thirdparty/re2:re2']
)

cc_test(
    name = 'format_string_parser_test',
    srcs = 'format_string_parser_test.cpp',
    deps = ':format_string_parser'
)

cc_library(
    name = 'proto_message_creator',
    srcs = ['error_printer.cpp',
            'proto_message_creator.cpp'],
    deps = [':codex_proto',
            '//common/base/string:string',
            '//thirdparty/protobuf:protoc',
            '//thirdparty/protobuf:protobuf',
            '//thirdparty/glog:glog']
)

cc_test(
    name = 'proto_message_creator_test',
    srcs = 'proto_message_creator_test.cpp',
    deps = ':proto_message_creator',
    testdata = 'field_formats.proto'
)

cc_library(
    name = "file_reader",
    srcs = "file_reader.cpp"
)

cc_library(
    name = 'recordio_reader',
    srcs = 'recordio_reader.cpp',
    deps = [':file_reader',
            '//common/file/recordio:recordio',
            '//common/base/string:string'],
    link_all_symbols = 1
)

cc_library(
    name = 'sstable_reader',
    srcs = 'sstable_reader.cpp',
    deps = [':file_reader',
            '//common/file/sstable:sstable',
            '//common/file:local_file'],
    link_all_symbols = 1
)

cc_library(
    name = 'raw_reader',
    srcs = 'raw_reader.cpp',
    deps = [':file_reader',
            '//common/file:local_file'],
    link_all_symbols = 1
)

cc_test(
    name = 'file_reader_test',
    srcs = 'file_reader_test.cpp',
    deps = [
        ':raw_reader',
        ':recordio_reader',
        ':sstable_reader',
        ':codex_test_proto',
        '//common/base/string:string',
    ]
)

cc_library(
    name = "field_parser",
    srcs = "field_parser.cpp",
    deps = [':codex_proto',
            ':proto_message_creator',
            '//common/base/string:string',
            '//thirdparty/glog:glog']
)

cc_test(
    name = 'field_parser_test',
    srcs = 'field_parser_test.cpp',
    deps = [':field_parser',
            ':format_string_parser',
            ':codex_test_proto'],
    testdata = 'codex_test.proto'
)

cc_binary(
    name = 'codex',
    srcs = ['codex.cpp'],
    deps = [':codex_proto',
            ':format_string_parser',
            ':proto_message_creator',
            ':field_parser',
            ':raw_reader',
            ':recordio_reader',
            ':sstable_reader',
            '//thirdparty/glog:glog',
            '//thirdparty/gflags:gflags']
)
