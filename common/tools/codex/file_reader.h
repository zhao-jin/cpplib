// Copyright (C) 2011 - Tencent Inc.
// Author: Xiang Wen <flyingwen@tencent.com>
//
// Define an interface to read input file. We see all input files as a
// collection of multi rows, each row contains a few fields. For example, every
// records in sstable contains a key and a value, then we see key as the first
// field, and value as the second. Besides, we provide reflection macros to make
// adding and using new file format more flexible.
//
// Example:
/*
   codex::FileReader *reader = CreateFileReaderObject("sstable");
   reader->Open("test.sstable");
   std::vector<StringPiece> fields;
   while (reader->Read(&fields)) {
   ...
   }
   reader->Close();
*/

#ifndef COMMON_TOOLS_CODEX_FILE_READER_H_
#define COMMON_TOOLS_CODEX_FILE_READER_H_

#include <string>
#include <vector>

#include "common/base/class_register.h"
#include "common/base/global_function_register.h"
#include "common/base/string/string_piece.h"

// namespace common {
namespace codex {

class FileReader {
public:
    virtual ~FileReader() {}

    // return false if something goes wrong
    virtual bool Open(const char *filename) = 0;

    // return false if something goes wrong
    virtual bool Close() = 0;

    // seek to a subset of records
    // when you call Seek(), you read from the first record which meets the
    // requirements, and after the last record of the subset, Read() return false.

    // seek to record(s) match @key
    virtual bool Seek(const StringPiece &key) = 0;

    // seek to record(s) range from @begin to @end
    virtual bool Seek(const StringPiece &begin, const StringPiece &end) = 0;


    // return false at the end of file
    virtual bool Read(std::vector<StringPiece> *fields) = 0;
};

CLASS_REGISTER_DEFINE_REGISTRY(FileReader, FileReader);

#define REGISTER_FILE_READER(format, class_name)\
    CLASS_REGISTER_OBJECT_CREATOR(FileReader, FileReader, #format, class_name)

FileReader *CreateFileReaderObject(const char *file_type);

}  // namespace codex
// } // namespace common

#endif  // COMMON_TOOLS_CODEX_FILE_READER_H_
