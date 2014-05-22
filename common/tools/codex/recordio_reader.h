// Copyright (C) 2011 - Tencent Inc.
// Author: Xiang Wen <flyingwen@tencent.com>
//
// Implementation of FileReader for recordio file

#ifndef COMMON_TOOLS_CODEX_RECORDIO_READER_H_
#define COMMON_TOOLS_CODEX_RECORDIO_READER_H_

#include <string>
#include <vector>

#include "common/base/scoped_ptr.h"
#include "common/base/string/string_piece.h"
#include "common/file/recordio/recordio.h"

#include "common/tools/codex/file_reader.h"

// namespace common {
namespace codex {

class RecordIOReader : public FileReader {
public:
    RecordIOReader();
    virtual ~RecordIOReader();

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
    scoped_ptr<RecordReader> m_record_reader;

    uint64_t m_record_count;
    uint64_t m_begin;
    uint64_t m_end;
};

}  // namespace codex
// } // namespace common

#endif  // COMMON_TOOLS_CODEX_RECORDIO_READER_H_
