// Copyright (c) 2011, Tencent Inc.
// Author: Xiang Wen <flyingwen@tencent.com>

#include "common/tools/codex/sstable_reader.h"

#include <string>
#include <vector>

#include "thirdparty/glog/logging.h"
#include "thirdparty/gtest/gtest.h"


// namespace common {
namespace codex {

REGISTER_FILE_READER(sstable, SSTableReader);


SSTableReader::SSTableReader() {
}

SSTableReader::~SSTableReader() {
}

bool SSTableReader::Open(const char *filename) {
    DCHECK(m_reader == NULL);

    m_filename = filename;

    // open for read and get iterator
    m_reader.reset(new sstable::SSTableReader(filename));

    if (m_reader->OpenFile() != 0) {
        LOG(ERROR) << "fail to open sstable file : " << m_filename;
        m_reader.reset();
        return false;
    }

    m_iter.reset(m_reader->NewIterator());
    m_iter->JumpToBlock(0);

    m_seek_key.clear();
    m_seek_begin.clear();
    m_seek_end.clear();

    return true;
}

bool SSTableReader::Close() {
    DCHECK(m_reader != NULL);

    m_iter.reset();
    if (m_reader->Close() != 0) {
        LOG(ERROR) << "sstable stream failed to close file : " << m_filename;
        return false;
    }

    m_reader.reset();

    return true;
}

bool SSTableReader::Seek(const StringPiece &key) {
    m_seek_key = key.as_string();
    m_iter->Seek(m_seek_key);
    return true;
}

bool SSTableReader::Seek(const StringPiece &begin, const StringPiece &end) {
    m_seek_begin = begin.as_string();
    m_seek_end = end.as_string();
    m_iter->Seek(m_seek_begin);
    return true;
}

bool SSTableReader::Read(std::vector<StringPiece> *fields) {
    DCHECK(m_reader != NULL);

    // end of file
    if (m_iter->Done()) {
        return false;
    }

    m_iter->GetKVPair(&m_key, &m_value);
    if (!m_seek_key.empty() && m_key != m_seek_key) {
        return false;
    }
    if (!m_seek_end.empty() && m_key > m_seek_end) {
        return false;
    }
    fields->clear();
    fields->push_back(StringPiece(m_key));
    fields->push_back(StringPiece(m_value));

    m_iter->Next();

    return true;
}

}  // namespace codex
// } // namespace common
