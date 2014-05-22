// Copyright (C) 2011 - Tencent Inc.
// Author: Xiang Wen <flyingwen@tencent.com>

#include "common/tools/codex/field_parser.h"

#include "common/base/singleton.h"
#include "common/base/string/string_number.h"
#include "thirdparty/glog/logging.h"

#include "common/tools/codex/proto_message_creator.h"

// namespace common {
namespace codex {

FieldParser *CreateFieldParser(const FieldFormat &format) {
    if (format.conversion() == "c") {
        // char
        return new CharacterFieldParser(format);
    } else if (format.conversion() == "d") {
        // machine dependent integer
        if (format.decorator() == "ll") {
            return new MachineIntegerFieldParser<int64_t>(format);
        } else if (format.decorator() == "h") {
            return new MachineIntegerFieldParser<int16_t>(format);
        } else if (format.decorator() == "hh") {
            return new MachineIntegerFieldParser<int8_t>(format);
        }
        return new MachineIntegerFieldParser<int32_t>(format);
    } else if (format.conversion() == "n") {
        // network integer
        if (format.decorator() == "ll") {
            return new NetIntegerFieldParser<int64_t>(format);
        } else if (format.decorator() == "h") {
            return new NetIntegerFieldParser<int16_t>(format);
        } else if (format.decorator() == "hh") {
            return new NetIntegerFieldParser<int8_t>(format);
        }
        return new NetIntegerFieldParser<int32_t>(format);
    } else if (format.conversion() == "f") {
        if (format.decorator() == "l") {
            return new FloatNumberFieldParser<double>(format);
        } else {
            return new FloatNumberFieldParser<float>(format);
        }
    } else if (format.conversion() == "s") {
        // string
        return new StringFieldParser(format);
    } else if (format.conversion() == "P") {
        return new MessageFieldParser(format);
    }

    return new EmptyFieldParser(format);
}

FieldParserSequence::FieldParserSequence(const FieldFormatSequence &formats) {
    for (int i = 0; i < formats.format_size(); ++i) {
        m_parsers.push_back(CreateFieldParser(formats.format(i)));
    }
}

FieldParserSequence::~FieldParserSequence() {
    for (size_t i = 0; i < m_parsers.size(); ++i) {
        delete m_parsers[i];
    }
}

void FieldParserSequence::ParseAndPrintField(const StringPiece &field, std::ostream *output) {
    int offset = 0;
    for (size_t i = 0; i < m_parsers.size(); ++i) {
        FieldParser *parser = m_parsers[i];
        offset += parser->ParseAndPrint(field, offset, output);
    }
}

FieldParser::FieldParser(const FieldFormat &format) {
    m_separation = format.separation();

    if (format.prefix().size() == 1) {
        m_has_prefix = true;
        m_prefix = format.prefix()[0];
    } else {
        m_has_prefix = false;
    }

    if (!StringToNumber(format.width(), &m_width)) {
        m_width = 0;
    }
}

void FieldParser::SetupCommonFormat(std::ostream *output) const {
    *output << m_separation;

    if (m_has_prefix) {
        output->fill(m_prefix);
    };
    output->width(m_width);
}

EmptyFieldParser::EmptyFieldParser(const FieldFormat &format)
    : FieldParser(format) {
        if (format.conversion() == "%") {
            m_is_escape = true;
        } else {
            m_is_escape = false;
        }
}

int EmptyFieldParser::ParseAndPrint(const StringPiece &field, int offset,
                            std::ostream *output) const {
    *output << m_separation;
    if (m_is_escape)
        *output << '%';

    return 0;
}

int CharacterFieldParser::ParseAndPrint(const StringPiece &field, int offset,
                                        std::ostream *output) const {
    CHECK_LE(offset + sizeof(char), field.size()) // NOLINT(runtime/sizeof)
        << "Failed to parse char";

    SetupCommonFormat(output);
    *output << *(field.data() + offset);

    return sizeof(char); // NOLINT(runtime/sizeof)
}

int StringFieldParser::ParseAndPrint(const StringPiece &field, int offset,
                                     std::ostream *output) const {
    CHECK_LE(offset, static_cast<int>(field.size()))
        << "Failed to parse string";

    SetupCommonFormat(output);
    *output << std::string(field.data() + offset, field.size() - offset);

    return field.size() - offset;
}

MessageFieldParser::MessageFieldParser(const FieldFormat &format)
    : FieldParser(format) {
    ProtoMessageCreator &creator = Singleton<ProtoMessageCreator>::Instance();
    m_message.reset(creator.CreateProtoMessage(format.attachment()));
    CHECK(m_message != NULL)
        << "Can not find prototype {" << format.attachment() << "}";
}

int MessageFieldParser::ParseAndPrint(const StringPiece &field, int offset,
                                      std::ostream *output) const {
    CHECK_LE(offset, static_cast<int>(field.size()))
        << "Failed to parse string";
    CHECK(m_message->ParseFromArray(field.data() + offset, field.size() - offset))
        << "Failed to parse proto message";

    SetupCommonFormat(output);
    *output << "{\n" << m_message->DebugString() << "}";

    return m_message->ByteSize();
}

}  // namespace codex
// } // namespace common
