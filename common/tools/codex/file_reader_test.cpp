// Copyright (c) 2011, Tencent Inc.
// Author: Xiang Wen <flyingwen@tencent.com>

#include "common/tools/codex/file_reader.h"

#include <fstream>
#include <string>
#include <vector>

#include "common/base/string/string_number.h"
#include "common/file/recordio/recordio.h"
#include "common/file/sstable/sstable_writer.h"
#include "common/system/io/file.h"
#include "thirdparty/gtest/gtest.h"

#include "common/tools/codex/codex_test.pb.h"

// namespace common {
namespace codex {

class RecordIOReaderTest : public testing::Test {
protected:
    virtual void SetUp() {
        // create data file
        std::ofstream stream(kFilename);
        RecordWriter *writer = new RecordWriter(&stream);
        for (int64_t i = kStartNumber; i < kEndNumber; ++i) {
            codex::TestProtoMessage message;
            message.set_number(i);
            message.set_text(NumberToString(i));
            writer->WriteMessage(message);
        }
        delete writer;
        stream.close();
    }

protected:
    static const char *kFilename;
    static const int64_t kStartNumber;
    static const int64_t kEndNumber;
};

const char *RecordIOReaderTest::kFilename = "test.recordio";
const int64_t RecordIOReaderTest::kStartNumber = 100;
const int64_t RecordIOReaderTest::kEndNumber = 200;

TEST_F(RecordIOReaderTest, Basic) {
    codex::FileReader *reader = CreateFileReaderObject("recordio");
    ASSERT_TRUE(reader != NULL);
    ASSERT_TRUE(reader->Open(kFilename));

    int64_t number = kStartNumber;
    std::vector<StringPiece> fields;
    while (reader->Read(&fields)) {
        codex::TestProtoMessage message;
        ASSERT_EQ(1U, fields.size());
        ASSERT_TRUE(message.ParseFromArray(fields[0].data(), fields[0].size()));
        ASSERT_EQ(number, message.number());
        ASSERT_EQ(NumberToString(number), message.text());

        fields.clear();
        ++number;
    }
    ASSERT_EQ(kEndNumber, number);
    ASSERT_TRUE(reader->Close());
    delete reader;
}

TEST_F(RecordIOReaderTest, SeekKey) {
    int64_t record_count = (kEndNumber - kStartNumber) / 2;

    codex::FileReader *reader = CreateFileReaderObject("recordio");
    ASSERT_TRUE(reader != NULL);
    ASSERT_TRUE(reader->Open(kFilename));
    ASSERT_TRUE(reader->Seek(NumberToString(record_count)));

    std::vector<StringPiece> fields;
    ASSERT_TRUE(reader->Read(&fields));
    ASSERT_EQ(1U, fields.size());
    codex::TestProtoMessage message;
    ASSERT_TRUE(message.ParseFromArray(fields[0].data(), fields[0].size()));
    ASSERT_EQ(kStartNumber + record_count, message.number());
    ASSERT_EQ(NumberToString(kStartNumber + record_count), message.text());
    ASSERT_FALSE(reader->Read(&fields));
    ASSERT_TRUE(reader->Close());
    delete reader;
}

TEST_F(RecordIOReaderTest, SeekRange) {
    int64_t begin_count = (kEndNumber - kStartNumber) / 4;
    int64_t end_count = (kEndNumber - kStartNumber) / 4 * 3;

    codex::FileReader *reader = CreateFileReaderObject("recordio");
    ASSERT_TRUE(reader != NULL);
    ASSERT_TRUE(reader->Open(kFilename));
    ASSERT_TRUE(reader->Seek(NumberToString(begin_count), NumberToString(end_count)));

    int64_t number = kStartNumber + begin_count;
    std::vector<StringPiece> fields;
    while (reader->Read(&fields)) {
        std::string text = NumberToString(number);

        ASSERT_EQ(1U, fields.size());

        codex::TestProtoMessage message;
        ASSERT_TRUE(message.ParseFromArray(fields[0].data(), fields[0].size()));
        ASSERT_EQ(number, message.number());
        ASSERT_EQ(text, message.text());

        fields.clear();
        ++number;
    }
    ASSERT_EQ(kStartNumber + end_count + 1, number);
    ASSERT_TRUE(reader->Close());
    delete reader;
}

class SSTableReaderTest : public testing::Test {
protected:
    virtual void SetUp() {
        // create data file
        sstable::SSTableOptions options;
        sstable::SSTableWriter writer(kFilename, options);
        writer.Open();
        for (int64_t i = kStartNumber; i < kEndNumber; ++i) {
            std::string text = NumberToString(i);

            codex::TestProtoMessage message;
            message.set_number(i);
            message.set_text(text);
            std::string message_string;
            message.SerializePartialToString(&message_string);

            writer.WriteRecord(text, message_string);
        }
        writer.Close();
    }

protected:
    static const char *kFilename;
    static const int64_t kStartNumber;
    static const int64_t kEndNumber;
};

const char *SSTableReaderTest::kFilename = "test.sstable";
const int64_t SSTableReaderTest::kStartNumber = 100;
const int64_t SSTableReaderTest::kEndNumber = 200;

TEST_F(SSTableReaderTest, Basic) {
    codex::FileReader *reader = CreateFileReaderObject("sstable");
    ASSERT_TRUE(reader != NULL);
    ASSERT_TRUE(reader->Open(kFilename));

    int64_t number = kStartNumber;
    std::vector<StringPiece> fields;
    while (reader->Read(&fields)) {
        std::string text = NumberToString(number);

        ASSERT_EQ(2U, fields.size());

        ASSERT_EQ(text, fields[0].as_string());

        codex::TestProtoMessage message;
        ASSERT_TRUE(message.ParseFromArray(fields[1].data(), fields[1].size()));
        ASSERT_EQ(number, message.number());
        ASSERT_EQ(text, message.text());

        fields.clear();
        ++number;
    }
    ASSERT_EQ(kEndNumber, number);
    ASSERT_TRUE(reader->Close());
    delete reader;
}

TEST_F(SSTableReaderTest, SeekKey) {
    int64_t key_number = (kStartNumber + kEndNumber) / 2;
    std::string key_text = NumberToString(key_number);

    codex::FileReader *reader = CreateFileReaderObject("sstable");
    ASSERT_TRUE(reader != NULL);
    ASSERT_TRUE(reader->Open(kFilename));
    ASSERT_TRUE(reader->Seek(key_text));

    std::vector<StringPiece> fields;
    ASSERT_TRUE(reader->Read(&fields));
    ASSERT_EQ(2U, fields.size());
    ASSERT_EQ(key_text, fields[0].as_string());
    codex::TestProtoMessage message;
    ASSERT_TRUE(message.ParseFromArray(fields[1].data(), fields[1].size()));
    ASSERT_EQ(key_number, message.number());
    ASSERT_EQ(key_text, message.text());
    ASSERT_FALSE(reader->Read(&fields));
    ASSERT_TRUE(reader->Close());
    delete reader;
}

TEST_F(SSTableReaderTest, SeekRange) {
    int64_t start_number = (kStartNumber + kEndNumber) / 2;
    int64_t end_number = kEndNumber - 1;

    codex::FileReader *reader = CreateFileReaderObject("sstable");
    ASSERT_TRUE(reader != NULL);
    ASSERT_TRUE(reader->Open(kFilename));
    ASSERT_TRUE(reader->Seek(NumberToString(start_number), NumberToString(end_number)));

    int64_t number = start_number;
    std::vector<StringPiece> fields;
    while (reader->Read(&fields)) {
        std::string text = NumberToString(number);

        ASSERT_EQ(2U, fields.size());

        ASSERT_EQ(text, fields[0].as_string());

        codex::TestProtoMessage message;
        ASSERT_TRUE(message.ParseFromArray(fields[1].data(), fields[1].size()));
        ASSERT_EQ(number, message.number());
        ASSERT_EQ(text, message.text());

        fields.clear();
        ++number;
    }
    ASSERT_EQ(kEndNumber, number);
    ASSERT_TRUE(reader->Close());
    delete reader;
}

static const char* kRawFileName = "raw_file.dat";
static const int kRawFileSize = 2048;
class RawReaderTest : public testing::Test {
protected:
    void SetUp() {
        scoped_ptr<File> file(File::Open(kRawFileName, "w"));
        EXPECT_TRUE(file.get() != NULL);
        std::string data(2048, 'A');
        EXPECT_EQ(kRawFileSize, file->Write(data.data(), data.size()));
    }
    void TearDown() {
        EXPECT_EQ(0, File::Remove(kRawFileName));
    }
};

TEST_F(RawReaderTest, Create) {
    scoped_ptr<codex::FileReader> reader(CreateFileReaderObject("raw"));
    ASSERT_TRUE(reader.get() != NULL);
}

TEST_F(RawReaderTest, Open) {
    scoped_ptr<codex::FileReader> reader(CreateFileReaderObject("raw"));
    EXPECT_FALSE(reader->Open("non-existed-file"));
    EXPECT_TRUE(reader->Open(kRawFileName));
}

TEST_F(RawReaderTest, Read) {
    scoped_ptr<codex::FileReader> reader(CreateFileReaderObject("raw"));
    EXPECT_TRUE(reader->Open(kRawFileName));
    std::vector<StringPiece> vs;
    EXPECT_TRUE(reader->Read(&vs));
    EXPECT_EQ(1U, vs.size());
    EXPECT_EQ(kRawFileSize, static_cast<int>(vs[0].size()));
}

TEST_F(RawReaderTest, Seek) {
    scoped_ptr<codex::FileReader> reader(CreateFileReaderObject("raw"));
    EXPECT_TRUE(reader->Open(kRawFileName));
    EXPECT_FALSE(reader->Seek("2049"));
    EXPECT_TRUE(reader->Seek("1024"));
    std::vector<StringPiece> vs;
    EXPECT_TRUE(reader->Read(&vs));
    EXPECT_EQ(1U, vs.size());
    EXPECT_EQ(1024, static_cast<int>(vs[0].size()));
}

TEST_F(RawReaderTest, SeekRange) {
    scoped_ptr<codex::FileReader> reader(CreateFileReaderObject("raw"));
    EXPECT_TRUE(reader->Open(kRawFileName));
    EXPECT_FALSE(reader->Seek("2049", "2046"));
    EXPECT_FALSE(reader->Seek("1024", "2096"));
    EXPECT_TRUE(reader->Seek("1024", "2046"));
    std::vector<StringPiece> vs;
    EXPECT_TRUE(reader->Read(&vs));
    EXPECT_EQ(1U, vs.size());
    EXPECT_EQ(1022, static_cast<int>(vs[0].size()));
}

}  // namespace codex
// } // namespace common
