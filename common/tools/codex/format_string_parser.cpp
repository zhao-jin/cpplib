// Copyright 2010 Tencent Inc.
// Author: Yi Wang <yiwang@tencet.com>

#include <stdio.h>

#include <string>

#include "common/tools/codex/field_formats.pb.h"
#include "common/tools/codex/format_string_parser.h"
#include "re2/re2.h"

// namespace common {
namespace codex {

using std::string;

// This regular expression encodes a field format.  For details about
// the format, please refer to comments in format_string_parser.h.
static const char* kFieldFormatsRegex =
    "([^%]*)"                           // anything that prefix an format
    "%"                                 // the start of an format or "%%"
    "([^%\\.hlcndfsP%]?)"               // nothing or a fill-in character
    "(\\d+)?"                           // nothing or few digits of width
    "(\\.\\d+)?"                        // nothing or precision
    "([hl]*)"                           // nothing or decorator
    "([cndfsP%])"                       // a (must-have) conversion
    "(\\{[\\.\\w]+\\})?";               // nothing or a proto message name


static bool CheckDecorator(const std::string& decorator) {
    if (decorator.size() > 0 &&
        decorator != "h"  &&
        decorator != "hh" &&
        decorator != "l"  &&
        decorator != "ll") {
        return false;
    }
    return true;
}

bool ParseFieldFormats(const std::string& format_string,
                        FieldFormatSequence* formats) {
    static const RE2 field_format_pattern(kFieldFormatsRegex);

    re2::StringPiece input(format_string);
    formats->Clear();

    FieldFormat field_format;
    std::string attachment;
    while (RE2::Consume(&input,
                        field_format_pattern,
                        field_format.mutable_separation(),
                        field_format.mutable_prefix(),
                        field_format.mutable_width(),
                        field_format.mutable_precision(),
                        field_format.mutable_decorator(),
                        field_format.mutable_conversion(),
                        &attachment)) {
        if (!CheckDecorator(field_format.decorator())) {
            fprintf(stderr, "Unknown decorator : %s\n", field_format.decorator().c_str());
            return false;
        }
        if (attachment != "") {
            // strip the surrounding "{}"
            field_format.set_attachment(attachment.substr(1, attachment.size() - 2));
        }
        formats->add_format()->CopyFrom(field_format);
    }

    if (input.size() > 0) {
        // Append the rest (imparsable part) of the input format string as
        // the separation of a new format.
        formats->add_format()->set_separation(input.as_string());
    }

    return true;
}

}  // namespace codex
// } // namespace common
