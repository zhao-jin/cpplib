// Copyright 2011, Tencent Inc.
// Author: Yi Wang <yiwang@tencent.com>

#include "common/base/string/concat.h"
#include "common/tools/codex/error_printer.h"
#include "thirdparty/glog/logging.h"

// namespace common {
namespace codex {

void ErrorPrinter::AddError(const std::string& filename, int line, int column,
                            const std::string& message) {
    StringAppend(&m_error_msg, filename);

    // Users typically expect 1-based line/column numbers, so we add 1
    // to each here.
    if (line != -1) {
        // Allow for both GCC- and Visual-Studio-compatible output.
        switch (m_format) {
            case ErrorPrinter::ERROR_FORMAT_GCC:
                StringAppend(&m_error_msg, ":", line + 1, ":", column + 1);
                break;
            case ErrorPrinter::ERROR_FORMAT_MSVS:
                StringAppend(&m_error_msg, "(", line + 1,
                             ") : error in column=", column + 1);
                break;
        }
    }

    StringAppend(&m_error_msg, ": ", message, "\n");
}

void ErrorPrinter::AddError(int line, int column, const std::string& message) {
    AddError("input", line, column, message);
}

}  // namespace codex
// } // namespace common
