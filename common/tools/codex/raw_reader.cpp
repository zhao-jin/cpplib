// Copyright (C) 2012 - Tencent Inc.
// Author: Chen Feng <hongchen@tencent.com>

#include "common/tools/codex/raw_reader.h"

#include <vector>

#include "common/base/string/string_number.h"
#include "common/file/file.h"
#include "thirdparty/glog/logging.h"

// namespace common {
namespace codex {

REGISTER_FILE_READER(raw, RawReader);

RawReader::RawReader() {
}

RawReader::~RawReader() {
}

bool RawReader::Open(const char *filename) {
    m_filename = filename;
    m_file.reset(File::Open(filename, File::ENUM_FILE_OPEN_MODE_R));
    if (m_file == NULL) {
        LOG(ERROR) << "fail to open recordio file: " << m_filename;
        return false;
    }

    m_file_size = File::GetSize(filename);
    if (m_file_size < 0) {
        LOG(ERROR) << "fail to get file size: " << m_filename;
        return false;
    }

    m_begin = 0;
    m_end = m_file_size;

    return true;
}

bool RawReader::Close() {
    m_file->Close();
    m_file.reset();

    return true;
}

bool RawReader::Seek(const StringPiece &key) {
    int64_t begin;
    if (!StringToNumber(key.as_string(), &begin)) {
        LOG(ERROR) << "Invalid begin: " << key;
        return false;
    }

    if (begin > m_file_size) {
        LOG(ERROR) << "Begin " << begin << " > file_size(" << m_file_size << ")";
        return false;
    }

    if (m_file->Seek(begin, SEEK_SET) < 0) {
        LOG(ERROR) << "Can't seek to " << begin;
    }

    m_begin = begin;
    return true;
}

bool RawReader::Seek(const StringPiece &begin, const StringPiece &end) {
    int64_t begin_offset;
    if (!StringToNumber(begin.as_string(), &begin_offset)) {
        LOG(ERROR) << "Invalid begin: " << begin;
        return false;
    }
    if (begin_offset > m_file_size) {
        LOG(ERROR) << "Begin " << begin_offset << " > file_size(" << m_file_size << ")";
        return false;
    }

    int64_t end_offset;
    if (!StringToNumber(end.as_string(), &end_offset)) {
        LOG(ERROR) << "Invalid end: " << end;
        return false;
    }
    if (end_offset > m_file_size) {
        LOG(ERROR) << "End: " << end_offset << " > file_size(" << m_file_size << ")";
        return false;
    }

    if (begin_offset > end_offset) {
        LOG(ERROR) << "Begin(" << begin_offset << ") > end(" << end_offset << ")";
        return false;
    }

    if (m_file->Seek(begin_offset, SEEK_SET) < 0) {
        LOG(ERROR) << "Can't seek to " << begin;
    }

    m_begin = begin_offset;
    m_end = end_offset;

    return true;
}

bool RawReader::Read(std::vector<StringPiece> *fields) {
    DCHECK(m_file.get() != NULL);

    m_buffer.resize(m_end - m_begin);
    if (!m_file->Read(&m_buffer[0], m_buffer.size())) {
        LOG(ERROR) << "Can't read file";
        return false;
    }

    fields->clear();
    fields->push_back(m_buffer);

    return true;
}

}  // namespace codex
// } // namespace common
