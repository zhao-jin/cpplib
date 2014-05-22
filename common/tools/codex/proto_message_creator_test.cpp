// Copyright 2010 Tencent Inc.
// Author: Yi Wang <yiwang@tencet.com>

#include "common/tools/codex/proto_message_creator.h"

#include <string>
#include <vector>

#include "common/tools/codex/field_formats.pb.h"
#include "thirdparty/gtest/gtest.h"
#include "thirdparty/protobuf/compiler/parser.h"
#include "thirdparty/protobuf/descriptor.h"
#include "thirdparty/protobuf/descriptor_database.h"
#include "thirdparty/protobuf/dynamic_message.h"
#include "thirdparty/protobuf/io/tokenizer.h"
#include "thirdparty/protobuf/io/zero_copy_stream_impl.h"

// namespace common {

// FIXME(phongchen): disable failed test
TEST(DISABLED_ProtoMessageCreatorTest, AGeneralCase) {
    using std::string;
    using std::vector;

    // Create a serialization of FieldFormatSequence for testing comparison.
    string serialization;
    codex::FieldFormatSequence formats;
    formats.add_format()->set_separation("key");
    formats.SerializeToString(&serialization);

    // We try to parse real proto files in our project.  Note that we have two
    // .proto files: format_option.proto defining codex.FieldFormat, and
    // format_options.proto defining codex.FieldFormatSequence, and
    // codex.FieldFormatSequence depends on codex.Format, and thus
    // format_options.proto has to import format_option.proto.  Here we parse
    // only format_options.proto, but not the imported file, format_option.proto.
    codex::ProtoMessageCreator creator;
    ASSERT_TRUE(creator.AddProtoSourceFile("./format_options.proto"));

    google::protobuf::Message* msg =
        creator.CreateProtoMessage("codex.FieldFormatSequence");
    ASSERT_TRUE(msg->ParseFromString(serialization));
    EXPECT_EQ("option { separation: \"key\" }", msg->ShortDebugString());
}

// } // namespace common
