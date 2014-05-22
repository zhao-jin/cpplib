// Copyright (c) 2011, Tencent Inc.
// Author: Xiang Wen <flyingwen@tencent.com>
//
// Implementation of FileReader for sstable file

#ifndef COMMON_TOOLS_CODEX_SSTABLE_READER_H_
#define COMMON_TOOLS_CODEX_SSTABLE_READER_H_

#include <string>
#include <vector>

#include "common/base/scoped_ptr.h"
#include "common/file/sstable/sstable_reader.h"
#include "common/file/sstable/sstable_reader_iterator.h"
#include "common/tools/codex/file_reader.h"

// namespace common {
namespace codex {

class SSTableReader : public FileReader {
public:
    SSTableReader();

    virtual ~SSTableReader();

    virtual bool Open(const char *filename);

    virtual bool Close();

    // seek to all record(s) whose key is "key"
    virtual bool Seek(const StringPiece &key);

    // seek to all record(s) whose key is between "begin" and "end"(inclusive).
    virtual bool Seek(const StringPiece &begin, const StringPiece &end);

    virtual bool Read(std::vector<StringPiece> *fileds);

private:
    std::string m_filename;
    std::string m_key;
    std::string m_value;

    std::string m_seek_key;
    std::string m_seek_begin;
    std::string m_seek_end;

    scoped_ptr<sstable::SSTableReader> m_reader;
    scoped_ptr<sstable::SSTableReaderIterator> m_iter;
};

}  // namespace codex
// } // namespace common

#endif  // COMMON_TOOLS_CODEX_SSTABLE_READER_H_
