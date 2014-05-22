// Copyright 2010 Tencent Inc.
// Author: Yi Wang <yiwang@tencet.com>

#include "common/tools/codex/format_string_parser.h"

#include "thirdparty/gtest/gtest.h"

// namespace common {

TEST(FormatStringParserTest, AGeneralCase) {
    codex::FieldFormatSequence formats;
    EXPECT_TRUE(codex::ParseFieldFormats(
            "key=%05hhd%010.10lf, value=%hn %P{mr.KeyValuePB}lalala.",
            &formats));
    EXPECT_EQ(5, formats.format_size());

    EXPECT_EQ("key=", formats.format(0).separation());
    EXPECT_EQ("0", formats.format(0).prefix());
    EXPECT_EQ("5", formats.format(0).width());
    EXPECT_EQ("", formats.format(0).precision());
    EXPECT_EQ("hh", formats.format(0).decorator());
    EXPECT_EQ("d", formats.format(0).conversion());
    EXPECT_EQ("", formats.format(0).attachment());

    EXPECT_EQ("", formats.format(1).separation());
    EXPECT_EQ("0", formats.format(1).prefix());
    EXPECT_EQ("10", formats.format(1).width());
    EXPECT_EQ(".10", formats.format(1).precision());
    EXPECT_EQ("l", formats.format(1).decorator());
    EXPECT_EQ("f", formats.format(1).conversion());
    EXPECT_EQ("", formats.format(1).attachment());

    EXPECT_EQ(", value=", formats.format(2).separation());
    EXPECT_EQ("", formats.format(2).prefix());
    EXPECT_EQ("", formats.format(2).width());
    EXPECT_EQ("", formats.format(2).precision());
    EXPECT_EQ("h", formats.format(2).decorator());
    EXPECT_EQ("n", formats.format(2).conversion());
    EXPECT_EQ("", formats.format(2).attachment());

    EXPECT_EQ(" ", formats.format(3).separation());
    EXPECT_EQ("", formats.format(3).prefix());
    EXPECT_EQ("", formats.format(3).width());
    EXPECT_EQ("", formats.format(3).precision());
    EXPECT_EQ("", formats.format(3).decorator());
    EXPECT_EQ("P", formats.format(3).conversion());
    EXPECT_EQ("mr.KeyValuePB", formats.format(3).attachment());

    EXPECT_EQ("lalala.", formats.format(4).separation());
    EXPECT_EQ("", formats.format(4).prefix());
    EXPECT_EQ("", formats.format(4).width());
    EXPECT_EQ("", formats.format(4).precision());
    EXPECT_EQ("", formats.format(4).decorator());
    EXPECT_EQ("", formats.format(4).conversion());
    EXPECT_EQ("", formats.format(4).attachment());
}

TEST(FormatStringParserTest, SeparationThenformat) {
    codex::FieldFormatSequence formats;
    EXPECT_TRUE(codex::ParseFieldFormats(
            "key=%05d",
            &formats));
    EXPECT_EQ(1, formats.format_size());

    EXPECT_EQ("key=", formats.format(0).separation());
    EXPECT_EQ("0", formats.format(0).prefix());
    EXPECT_EQ("5", formats.format(0).width());
    EXPECT_EQ("", formats.format(0).precision());
    EXPECT_EQ("", formats.format(0).decorator());
    EXPECT_EQ("d", formats.format(0).conversion());
    EXPECT_EQ("", formats.format(0).attachment());
}

TEST(FormatStringParserTest, formatThenSeparation) {
    codex::FieldFormatSequence formats;
    EXPECT_TRUE(codex::ParseFieldFormats(
            "%05d.\n",
            &formats));
    EXPECT_EQ(2, formats.format_size());

    EXPECT_EQ("", formats.format(0).separation());
    EXPECT_EQ("0", formats.format(0).prefix());
    EXPECT_EQ("5", formats.format(0).width());
    EXPECT_EQ("", formats.format(0).precision());
    EXPECT_EQ("", formats.format(0).decorator());
    EXPECT_EQ("d", formats.format(0).conversion());
    EXPECT_EQ("", formats.format(0).attachment());

    EXPECT_EQ(".\n", formats.format(1).separation());
    EXPECT_EQ("", formats.format(1).prefix());
    EXPECT_EQ("", formats.format(1).width());
    EXPECT_EQ("", formats.format(1).precision());
    EXPECT_EQ("", formats.format(1).decorator());
    EXPECT_EQ("", formats.format(1).conversion());
    EXPECT_EQ("", formats.format(1).attachment());
}

TEST(FormatStringParserTest, UnknownDecorator) {
    codex::FieldFormatSequence formats;

    EXPECT_FALSE(codex::ParseFieldFormats(
            "%hld", &formats));
    EXPECT_FALSE(codex::ParseFieldFormats(
            "%lhd", &formats));
    EXPECT_FALSE(codex::ParseFieldFormats(
            "%hhhd", &formats));
    EXPECT_FALSE(codex::ParseFieldFormats(
            "%llld", &formats));

    EXPECT_TRUE(codex::ParseFieldFormats(
            "%hhd", &formats));
    EXPECT_EQ(1, formats.format_size());
    EXPECT_EQ("hh", formats.format(0).decorator());

    EXPECT_TRUE(codex::ParseFieldFormats(
            "%hd", &formats));
    EXPECT_EQ(1, formats.format_size());
    EXPECT_EQ("h", formats.format(0).decorator());

    EXPECT_TRUE(codex::ParseFieldFormats(
            "%d", &formats));
    EXPECT_EQ(1, formats.format_size());
    EXPECT_EQ("", formats.format(0).decorator());

    EXPECT_TRUE(codex::ParseFieldFormats(
            "%ld", &formats));
    EXPECT_EQ(1, formats.format_size());
    EXPECT_EQ("l", formats.format(0).decorator());

    EXPECT_TRUE(codex::ParseFieldFormats(
            "%lld", &formats));
    EXPECT_EQ(1, formats.format_size());
    EXPECT_EQ("ll", formats.format(0).decorator());
}

// } // namespace common
