// Copyright (C) 2011 - Tencent Inc.
// Author: Xiang Wen <flyingwen@tencent.com>

#include "common/tools/codex/file_reader.h"

// namespace common {
namespace codex {

FileReader *CreateFileReaderObject(const char *file_type) {
    FileReader *reader = CLASS_REGISTER_CREATE_OBJECT(FileReader, file_type);
    return reader;
}

}  // namespace codex
// } // namespace common
