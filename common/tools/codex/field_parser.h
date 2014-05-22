// Copyright (C) 2011 - Tencent Inc.
// Author: Xiang Wen <flyingwen@tencent.com>
//
// Define classes to parse contents in fields read from FileReader. A field in
// file may need multiple parsers, each parser returns comsumed bytes it used to
// give formatted output.
// FieldParser is an abstract base class for all other clasess defined in this
// file.
// FieldParserSequence is an sequence of field parsers, corresponding to
// FieldFormatSequence, see comments in format_string_parser.h
//
// example:
/*
   string format_string = "%c%d%n%f%s";
   stringstream content;

   char c = 'a';
   int32_t d = 12345;
   int32_t n = ByteOrder::LocalToNet(d);
   float f = 1.2345;
   char s[] = "hello";

   content.write(&c, sizeof(c));
   content.write(&d, sizeof(d));
   content.write(&n, sizeof(n));
   content.write(&f, sizeof(f));
   content.write(s, strlen(s));

   FieldFormatSequence formats;
   ParseFieldFormats(format_string, &formats);
   FieldParserSequence parsers(formats);
   parsers.ParseAndPrintField(StringPiece(content.str()), &cout); // "a12345123451.2345hello"
   */

#ifndef COMMON_TOOLS_CODEX_FIELD_PARSER_H_
#define COMMON_TOOLS_CODEX_FIELD_PARSER_H_

#include <ostream>
#include <string>
#include <vector>

#include "common/base/byte_order.h"
#include "common/base/scoped_ptr.h"
#include "common/base/string/string_number.h"
#include "common/base/string/string_piece.h"
#include "common/system/memory/unaligned.h"
#include "thirdparty/glog/logging.h"

#include "common/tools/codex/field_formats.pb.h"


namespace google {
namespace protobuf {
class Message;
}
}

// namespace common {
namespace codex {

// Base class for all parser class. All FieldParser should be created by global
// function CreateFieldParser().
//
class FieldParser {
public:
    explicit FieldParser(const FieldFormat &format);

    virtual ~FieldParser() {}

    // parse the content of field, and print formatted string to output
    // return consumed bytes
    virtual int ParseAndPrint(const StringPiece &field, int offset,
                              std::ostream *output) const = 0;

protected:
    void SetupCommonFormat(std::ostream *output) const;

protected:
    std::string m_separation;
    bool m_has_prefix;
    char m_prefix;
    int m_width;
};
FieldParser *CreateFieldParser(const FieldFormat &format);

// A sequence of field parsers, is responsible for parsing the whole field.
//
class FieldParserSequence {
public:
    explicit FieldParserSequence(const FieldFormatSequence &formats);
    ~FieldParserSequence();

    void ParseAndPrintField(const StringPiece &field, std::ostream *output);

private:
    std::vector<FieldParser*> m_parsers;
};


// In the case that only separation is given in format or the conversion is %%
//
class EmptyFieldParser : public FieldParser {
public:
    explicit EmptyFieldParser(const FieldFormat &format);

    virtual ~EmptyFieldParser() {}

    virtual int ParseAndPrint(const StringPiece &field, int offset,
                              std::ostream *output) const;

private:
    bool m_is_escape;
    // in the case of "%%"
};


// When the field contains a char
//
class CharacterFieldParser : public FieldParser {
public:
    explicit CharacterFieldParser(const FieldFormat &format)
        : FieldParser(format) {}

    virtual ~CharacterFieldParser() {}

    virtual int ParseAndPrint(const StringPiece &field, int offset,
                              std::ostream *output) const;
};


// When the field contains a string
//
class StringFieldParser : public FieldParser {
public:
    explicit StringFieldParser(const FieldFormat &format)
        : FieldParser(format) {}

    virtual ~StringFieldParser() {}

    virtual int ParseAndPrint(const StringPiece &field, int offset,
                              std::ostream *output) const;
};


// When the field contains an architecture-dependant integer
//
template <typename IntegerType>
class MachineIntegerFieldParser : public FieldParser {
public:
    explicit MachineIntegerFieldParser(const FieldFormat &format)
        : FieldParser(format) {}

    virtual ~MachineIntegerFieldParser() {}

    virtual int ParseAndPrint(const StringPiece &field, int offset,
                              std::ostream *output) const;
};


// When the field contains a network integer
//
template <typename IntegerType>
class NetIntegerFieldParser : public FieldParser {
public:
    explicit NetIntegerFieldParser(const FieldFormat &format)
        : FieldParser(format) {}

    virtual ~NetIntegerFieldParser() {}

    virtual int ParseAndPrint(const StringPiece &field, int offset,
                              std::ostream *output) const;
};


// When the field contains a float point number
//
template <typename T>
class FloatNumberFieldParser : public FieldParser {
public:
    explicit FloatNumberFieldParser(const FieldFormat &format)
        : FieldParser(format) {
            if (StringToNumber(format.precision(), &m_precision) && m_precision > 0) {
                m_has_precision = true;
            } else {
                m_has_precision = false;
            }
        }

    virtual ~FloatNumberFieldParser() {}

    virtual int ParseAndPrint(const StringPiece &field, int offset,
                              std::ostream *output) const;

private:
    bool m_has_precision;
    int m_precision;
};


// When the field contains the serialization of a proto message
//
class MessageFieldParser : public FieldParser {
public:
    explicit MessageFieldParser(const FieldFormat &format);

    virtual ~MessageFieldParser() {}

    virtual int ParseAndPrint(const StringPiece &field, int offset,
                              std::ostream *output) const;

private:
    scoped_ptr<google::protobuf::Message> m_message;
};


/************************ template member function *****************/

template <typename IntegerType>
int MachineIntegerFieldParser<IntegerType>::ParseAndPrint(const StringPiece &field, int offset,
                                                          std::ostream *output) const {
    CHECK_LE(offset + sizeof(IntegerType), field.size())
        << "Failed to parse machine integer";

    SetupCommonFormat(output);

    IntegerType integer = GetUnaligned<IntegerType>(field.data() + offset);
    *output << static_cast<int64_t>(integer);

    return sizeof(IntegerType);
}

template <typename IntegerType>
int NetIntegerFieldParser<IntegerType>::ParseAndPrint(const StringPiece &field, int offset,
                                                      std::ostream *output) const {
    CHECK_LE(offset + sizeof(IntegerType), field.size())
        << "Failed to parse net integer";

    SetupCommonFormat(output);

    IntegerType integer = GetUnaligned<IntegerType>(field.data() + offset);
    ByteOrder::NetToLocal(&integer);
    *output << static_cast<int64_t>(integer);

    return sizeof(IntegerType);
}

template <typename FloatType>
int FloatNumberFieldParser<FloatType>::ParseAndPrint(const StringPiece &field, int offset,
                                                     std::ostream *output) const {
    CHECK_LE(offset + sizeof(FloatType), field.size())
        << "Failed to parse float number";

    SetupCommonFormat(output);
    if (m_has_precision) {
        output->precision(m_precision);
    }

    FloatType number = GetUnaligned<FloatType>(field.data() + offset);
    *output << number;

    return sizeof(FloatType);
}

}  // namespace codex
// } // namespace common

#endif  // COMMON_TOOLS_CODEX_FIELD_PARSER_H_
