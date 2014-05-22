// Copyright (c) 2011, Tencent Inc.
// Author: Xiang Wen <flyingwen@tencent.com>

#include "common/tools/codex/field_parser.h"

#include <string.h>
#include <sstream>
#include <string>

#include "common/base/byte_order.h"
#include "common/base/scoped_ptr.h"
#include "common/base/singleton.h"
#include "common/base/string/concat.h"
#include "common/base/string/string_number.h"
#include "common/base/string/string_piece.h"
#include "thirdparty/gtest/gtest.h"

#include "common/tools/codex/codex_test.pb.h"
#include "common/tools/codex/field_formats.pb.h"
#include "common/tools/codex/format_string_parser.h"
#include "common/tools/codex/proto_message_creator.h"

// namespace common {
namespace codex {

TEST(FieldParserTest, EmptyField) {
    std::stringstream stream;
    FieldFormat format;
    scoped_ptr<FieldParser> parser;

    // only separation
    format.Clear();
    format.set_separation("hello world");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    ASSERT_EQ(0, parser->ParseAndPrint(StringPiece(), 0, &stream));
    ASSERT_EQ("hello world", stream.str());

    // only %
    format.Clear();
    format.set_conversion("%");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    ASSERT_EQ(0, parser->ParseAndPrint(StringPiece(), 0, &stream));
    ASSERT_EQ("%", stream.str());
}

TEST(FieldParserTest, Char) {
    std::stringstream stream;
    FieldFormat format;
    scoped_ptr<FieldParser> parser;
    char c = 'x';

    // basic output
    format.Clear();
    format.set_conversion("c");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    ASSERT_EQ(1, parser->ParseAndPrint(StringPiece(&c, 1), 0, &stream));
    ASSERT_EQ("x", stream.str());

    // format output
    format.Clear();
    format.set_separation("###");
    format.set_prefix(".");
    format.set_width("4");
    format.set_conversion("c");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    ASSERT_EQ(1, parser->ParseAndPrint(StringPiece(&c, 1), 0, &stream));
    ASSERT_EQ("###...x", stream.str());
}

TEST(FieldParserTest, String) {
    std::stringstream stream;
    FieldFormat format;
    scoped_ptr<FieldParser> parser;
    char text[] = "hello world";

    // basic output
    format.Clear();
    format.set_conversion("s");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    ASSERT_EQ(11, parser->ParseAndPrint(StringPiece(text, strlen(text)), 0, &stream));
    ASSERT_EQ("hello world", stream.str());

    // format output
    format.Clear();
    format.set_separation("###");
    format.set_prefix(".");
    format.set_width("16");
    format.set_conversion("s");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    ASSERT_EQ(11, parser->ParseAndPrint(StringPiece(text, strlen(text)), 0, &stream));
    ASSERT_EQ("###.....hello world", stream.str());
}

TEST(FieldParserTest, MachineInteger) {
    std::stringstream stream;
    FieldFormat format;
    scoped_ptr<FieldParser> parser;

    // basic output: %d
    format.Clear();
    format.set_conversion("d");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int32_t int_d = 2011;
    ASSERT_EQ(4, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_d), sizeof(int_d)), 0, &stream));
    ASSERT_EQ("2011", stream.str());

    // basic output: %ld
    format.Clear();
    format.set_conversion("d");
    format.set_decorator("l");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int32_t int_ld = 2011;
    ASSERT_EQ(4, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_ld), sizeof(int_ld)), 0, &stream));
    ASSERT_EQ("2011", stream.str());

    // basic output: %lld
    format.Clear();
    format.set_conversion("d");
    format.set_decorator("ll");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int64_t int_lld = 2011;
    ASSERT_EQ(8, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_lld), sizeof(int_lld)), 0, &stream));
    ASSERT_EQ("2011", stream.str());

    // basic output: %hd
    format.Clear();
    format.set_conversion("d");
    format.set_decorator("h");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int16_t int_hd = 2011;
    ASSERT_EQ(2, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_hd), sizeof(int_hd)), 0, &stream));
    ASSERT_EQ("2011", stream.str());

    // basic output: %hhd
    format.Clear();
    format.set_conversion("d");
    format.set_decorator("hh");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int8_t int_hhd = 11;
    ASSERT_EQ(1, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_hhd), sizeof(int_hhd)), 0, &stream));
    ASSERT_EQ("11", stream.str());

    // format output
    format.Clear();
    format.set_separation("=");
    format.set_prefix("0");
    format.set_width("5");
    format.set_conversion("d");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int_d = 2011;
    ASSERT_EQ(4, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_d), sizeof(int_d)), 0, &stream));
    ASSERT_EQ("=02011", stream.str());
}

TEST(FieldParserTest, NetInteger) {
    std::stringstream stream;
    FieldFormat format;
    scoped_ptr<FieldParser> parser;

    // basic output: %n
    format.Clear();
    format.set_conversion("n");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int32_t int_n = ByteOrder::LocalToNet(static_cast<int32_t>(2011));
    ASSERT_EQ(4, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_n), sizeof(int_n)), 0, &stream));
    ASSERT_EQ("2011", stream.str());

    // basic output: %ln
    format.Clear();
    format.set_conversion("n");
    format.set_decorator("l");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int32_t int_ln = ByteOrder::LocalToNet(static_cast<int32_t>(2011));
    ASSERT_EQ(4, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_ln), sizeof(int_ln)), 0, &stream));
    ASSERT_EQ("2011", stream.str());

    // basic output: %lln
    format.Clear();
    format.set_conversion("n");
    format.set_decorator("ll");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int64_t int_lln = ByteOrder::LocalToNet(static_cast<int64_t>(2011));
    ASSERT_EQ(8, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_lln), sizeof(int_lln)), 0, &stream));
    ASSERT_EQ("2011", stream.str());

    // basic output: %hn
    format.Clear();
    format.set_conversion("n");
    format.set_decorator("h");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int16_t int_hn = ByteOrder::LocalToNet(static_cast<int16_t>(2011));
    ASSERT_EQ(2, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_hn), sizeof(int_hn)), 0, &stream));
    ASSERT_EQ("2011", stream.str());

    // basic output: %hhn
    format.Clear();
    format.set_conversion("n");
    format.set_decorator("hh");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int8_t int_hhn = ByteOrder::LocalToNet(static_cast<int8_t>(11));
    ASSERT_EQ(1, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_hhn), sizeof(int_hhn)), 0, &stream));
    ASSERT_EQ("11", stream.str());

    // format output
    format.Clear();
    format.set_separation("=");
    format.set_prefix("0");
    format.set_width("5");
    format.set_conversion("n");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    int_n = ByteOrder::LocalToNet(static_cast<int32_t>(2011));
    ASSERT_EQ(4, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&int_n), sizeof(int_n)), 0, &stream));
    ASSERT_EQ("=02011", stream.str());
}

TEST(FieldParserTest, FloatNumber) {
    std::stringstream stream;
    FieldFormat format;
    scoped_ptr<FieldParser> parser;

    // basic output: %f
    format.Clear();
    format.set_conversion("f");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    float float_f = 3.1415926;
    ASSERT_EQ(4, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&float_f), sizeof(float_f)), 0, &stream));
    ASSERT_EQ("3.14159", stream.str());

    // basic output: %lf
    format.Clear();
    format.set_conversion("f");
    format.set_decorator("l");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    double float_lf = 3.1415926;
    ASSERT_EQ(8, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&float_lf), sizeof(float_lf)), 0, &stream));
    ASSERT_EQ("3.14159", stream.str());

    // format output
    format.Clear();
    format.set_separation("=");
    format.set_prefix("0");
    format.set_width("10");
    format.set_precision("7");
    format.set_conversion("f");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);
    stream.str("");
    float_f = 3.1415926;
    ASSERT_EQ(4, parser->ParseAndPrint(
            StringPiece(reinterpret_cast<char*>(&float_f), sizeof(float_f)), 0, &stream));
    ASSERT_EQ("=003.141593", stream.str());
}

TEST(FieldParserTest, ProtoMessage) {
    std::stringstream stream;
    FieldFormat format;
    scoped_ptr<FieldParser> parser;

    ProtoMessageCreator &creator = Singleton<ProtoMessageCreator>::Instance();
    creator.AddImportPath("./");
    creator.AddProtoSourceFile("codex_test.proto");

    // test basic output only
    format.Clear();
    format.set_conversion("P");
    format.set_attachment("codex.TestProtoMessage");
    parser.reset(CreateFieldParser(format));
    ASSERT_TRUE(parser != NULL);

    TestProtoMessage message;
    message.set_number(112);
    message.set_text("hello world");
    std::string content = message.SerializeAsString();
    stream.str("");
    ASSERT_EQ(message.ByteSize(), parser->ParseAndPrint(StringPiece(content), 0, &stream));
    ASSERT_EQ("{\n" + message.DebugString() + "}", stream.str());
}

TEST(FieldParserTest, UnifyTest) {
    std::string format_string = "%c%d%n%f%s";
    std::stringstream content;
    std::stringstream output;

    char c = 'a';
    int32_t d = 12345;
    int32_t n = ByteOrder::LocalToNet(d);
    float f = 1.2345;
    char s[] = "hello";

    content.write(&c, sizeof(c));
    content.write(reinterpret_cast<char*>(&d), sizeof(d));
    content.write(reinterpret_cast<char*>(&n), sizeof(n));
    content.write(reinterpret_cast<char*>(&f), sizeof(f));
    content.write(s, strlen(s));

    FieldFormatSequence formats;
    ASSERT_EQ(true, ParseFieldFormats(format_string, &formats));
    FieldParserSequence parsers(formats);
    parsers.ParseAndPrintField(StringPiece(content.str()), &output);
    ASSERT_EQ("a12345123451.2345hello", output.str());
}

}  // namespace codex
// } // namespace common
