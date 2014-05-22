// Copyright 2011, Tencent Inc.
// Author: Yi Wang <yiwang@tencent.com>
//
// Define class ErrorPrinter, which is supposed to be used by
// proto_message_creator.{h,cc} in parsing/compiling .proto files.
// This class collects errors into a buffer, which can be accessed
// later.
//
#ifndef COMMON_TOOLS_CODEX_ERROR_PRINTER_H_
#define COMMON_TOOLS_CODEX_ERROR_PRINTER_H_

#include <string>

#include "protobuf/compiler/importer.h"
#include "protobuf/io/tokenizer.h"

// namespace common {
namespace codex {

class ErrorPrinter : public google::protobuf::compiler::MultiFileErrorCollector,
                     public google::protobuf::io::ErrorCollector {
public:
    enum ErrorFormat {
        ERROR_FORMAT_GCC,
        ERROR_FORMAT_MSVS
    };

    explicit ErrorPrinter(ErrorFormat format) : m_format(format) {}
    ~ErrorPrinter() {}

    const std::string& GetError() const { return m_error_msg; }

    // Implements MultiFileErrorCollector.
    void AddError(const std::string& filename, int line, int column,
                  const std::string& message);

    // Implements ErrorCollector.
    void AddError(int line, int column, const std::string& message);

private:
    ErrorFormat m_format;
    std::string m_error_msg;
};

}  // namespace codex
// } // namespace common

#endif  // COMMON_TOOLS_CODEX_ERROR_PRINTER_H_
