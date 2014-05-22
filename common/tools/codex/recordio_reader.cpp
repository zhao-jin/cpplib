// Copyright (C) 2011 - Tencent Inc.
// Author: Xiang Wen <flyingwen@tencent.com>

#include "common/tools/codex/recordio_reader.h"

#include <vector>

#include "common/base/string/string_number.h"
#include "common/file/file.h"
#include "thirdparty/glog/logging.h"

// namespace common {
namespace codex {

REGISTER_FILE_READER(recordio, RecordIOReader);

RecordIOReader::RecordIOReader() {
}

RecordIOReader::~RecordIOReader() {
    DCHECK(m_record_reader == NULL);
}

bool RecordIOReader::Open(const char *filename) {
    DCHECK(m_record_reader == NULL);

    m_filename = filename;
    m_file.reset(File::Open(filename, File::ENUM_FILE_OPEN_MODE_R));
    if (m_file == NULL) {
        LOG(ERROR) << "fail to open recordio file: " << m_filename;
        return false;
    }

    m_record_reader.reset(new RecordReader(m_file.get()));

    m_record_count = 0;
    m_begin = 0;
    m_end = -1;

    return true;
}

bool RecordIOReader::Close() {
    m_record_reader.reset();
    m_file->Close();
    m_file.reset();

    return true;
}

bool RecordIOReader::Seek(const StringPiece &key) {
    if (!StringToNumber(key.as_string(), &m_begin))
        return false;
    m_end = m_begin;

    const char *data;
    int32_t size;
    while (m_record_count < m_begin &&
           m_record_reader->ReadRecord(&data, &size))
        ++m_record_count;

    return true;
}

bool RecordIOReader::Seek(const StringPiece &begin, const StringPiece &end) {
    if (!StringToNumber(begin.as_string(), &m_begin))
        return false;
    if (!StringToNumber(end.as_string(), &m_end))
        return false;

    const char *data;
    int32_t size;
    while (m_record_count < m_begin &&
           m_record_reader->ReadRecord(&data, &size))
        ++m_record_count;

    return true;
}

bool RecordIOReader::Read(std::vector<StringPiece> *fields) {
    DCHECK(m_record_reader.get() != NULL);

    if (m_record_count > m_end)
        return false;

    const char *data;
    int32_t size;
    if (!m_record_reader->ReadRecord(&data, &size)) {
        return false;
    }
    ++m_record_count;

    fields->clear();
    fields->push_back(StringPiece(data, size));

    return true;
}

}  // namespace codex
// } // namespace common
