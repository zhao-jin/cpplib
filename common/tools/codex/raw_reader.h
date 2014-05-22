// Copyright (C) 2012 - Tencent Inc.
// Author: Chen Feng <hongchen@tencent.com>
//
// Implementation of FileReader for recordio file

#ifndef COMMON_TOOLS_CODEX_RAW_READER_H_
#define COMMON_TOOLS_CODEX_RAW_READER_H_

#include <string>
#include <vector>

#include "common/base/scoped_ptr.h"
#include "common/base/string/string_piece.h"
#include "common/file/file.h"
#include "common/tools/codex/file_reader.h"

// namespace common {
namespace codex {

class RawReader : public FileReader {
public:
    RawReader();
    virtual ~RawReader();

    virtual bool Open(const char *filename);

    virtual bool Close();

    // read the [@key]th record, @key start from 0
    virtual bool Seek(const StringPiece &key);

    // read the [@begin, @end]th record, @begin start from 0
    virtual bool Seek(const StringPiece &begin, const StringPiece &end);

    // Read fields of next record in file
    virtual bool Read(std::vector<StringPiece> *fields);

private:
    std::string m_filename;
    scoped_ptr<common::File> m_file;
    std::string m_buffer;
    int64_t m_file_size;
    int64_t m_begin;
    int64_t m_end;
};

}  // namespace codex
// } // namespace common

#endif  // COMMON_TOOLS_CODEX_RAW_READER_H_
