// Copyright (c) 2011, Tencent Inc. All rights reserved.
// Author:  Wen Jie <welkinwen@tencent.com>
// Created: 2011-06-02

#ifndef COMMON_WEB_HASH_VALUE_H
#define COMMON_WEB_HASH_VALUE_H

#include <assert.h>
#include <string.h>
#include <ostream>
#include <string>

#include "common/base/annotation.h"

namespace web {

template <size_t BYTE_SIZE>
class HashValue
{
public:
    HashValue()
    {
        memset(m_value, 0, sizeof(m_value));
    }

    HashValue(const void * value, size_t size)
    {
        bool status = SetValue(value, size);
        assert(status);
        IgnoreUnused(status);
    }

    explicit HashValue(const char * str)
    {
        bool status = ParseFromHexString(str);
        assert(status);
        IgnoreUnused(status);
    }

    inline void Clear()
    {
        memset(m_value, 0, sizeof(m_value));
    }

    inline bool SetValue(const void * value, size_t size = BYTE_SIZE, size_t offset = 0)
    {
        assert(value != NULL);

        if ((offset + size) > sizeof(m_value))
        {
            return false;
        }

        memcpy(m_value + offset, value, size);

        return true;
    }

    bool ParseFromHexString(const char * str)
    {
        assert(str != NULL);

        memset(m_value, 0, sizeof(m_value));

        size_t len = strlen(str);
        if (len > sizeof(m_value) * 2)
        {
            return false;
        }

        unsigned char ch;
        unsigned int read_pos, write_pos;

        if ((len % 2) == 0)
        {
            write_pos = sizeof(m_value) - len / 2;
            read_pos = 0;
        }
        else
        {
            write_pos = sizeof(m_value) - len / 2 - 1;
            if (!Hex2Char('0', str[0], &ch))
            {
                return false;
            }
            m_value[write_pos++] = ch;
            read_pos = 1;
        }

        while (read_pos < len)
        {
            if (!Hex2Char(str[read_pos], str[read_pos + 1], &ch))
            {
                memset(m_value, 0, sizeof(m_value));
                return false;
            }
            m_value[write_pos++] = ch;
            read_pos += 2;
        }

        return true;
    }

    inline unsigned char * AsBytes() const
    {
        return const_cast<unsigned char *>(m_value);
    }

    inline std::string ToString() const
    {
        return std::string(reinterpret_cast<const char *>(m_value), sizeof(m_value));
    }

    std::string ToHexString(bool no_padding = true) const
    {
        std::string str_hex;
        unsigned int read_pos, write_pos;
        size_t len = sizeof(m_value);
        static const char HEX_CHAR[] = "0123456789abcdef";

        read_pos = write_pos = 0;
        str_hex.resize(len * 2);

        if (no_padding)
        {
            while (read_pos < len)
            {
                if (static_cast<int>(m_value[read_pos]) != 0)
                {
                    if (((m_value[read_pos] & 0xF0) >> 4) == 0)
                    {
                        str_hex[write_pos++] = HEX_CHAR[m_value[read_pos] & 0x0F];
                        ++read_pos;
                    }
                    break;
                }
                ++read_pos;
            }
        }

        while (read_pos < len)
        {
            str_hex[write_pos++] = HEX_CHAR[(m_value[read_pos] & 0xF0) >> 4];
            str_hex[write_pos++] = HEX_CHAR[m_value[read_pos] & 0x0F];
            ++read_pos;
        }
        str_hex.resize(write_pos);

        return str_hex.empty() ? "0" : str_hex;
    }

    std::ostream & operator << (std::ostream & os) const
    {
        os << ToHexString();
        return os;
    }

    inline bool IsZero() const
    {
        static const unsigned char ZERO[BYTE_SIZE] = { 0 };
        return (memcmp(m_value, ZERO, sizeof(m_value)) == 0);
    }

    inline bool operator == (const HashValue & rhs) const
    {
        return (memcmp(m_value, rhs.m_value, sizeof(m_value)) == 0);
    }

    inline bool operator != (const HashValue & rhs) const
    {
        return (memcmp(m_value, rhs.m_value, sizeof(m_value)) != 0);
    }

    inline bool operator < (const HashValue & rhs) const
    {
        return (memcmp(m_value, rhs.m_value, sizeof(m_value)) < 0);
    }

    inline bool operator > (const HashValue & rhs) const
    {
        return (memcmp(m_value, rhs.m_value, sizeof(m_value)) > 0);
    }

private:
    inline bool Hex2Char(const char hex_high, const char hex_low, unsigned char * ch) const
    {
        int high = Hex2Int(hex_high);
        int low = Hex2Int(hex_low);

        if ((high == -1) || (low == -1))
        {
            return false;
        }
        *ch = static_cast<unsigned char>(high * 16 + low);

        return true;
    }

    inline int Hex2Int(const char ch) const
    {
        if (ch >= '0' && ch <= '9')
        {
            return (ch - '0');
        }
        else if (ch >= 'a' && ch <= 'f')
        {
            return (ch - 'a' + 10);
        }
        else if (ch >= 'A' && ch <= 'F')
        {
            return (ch - 'A' + 10);
        }

        return -1;
    }

private:
    unsigned char m_value[BYTE_SIZE];
};

typedef HashValue<64/8> Md5HashValue64;
typedef HashValue<96/8> Md5HashValue96;
typedef HashValue<128/8> Md5HashValue;
typedef HashValue<160/8> Sha1HashValue;

} // namespace web

#endif // COMMON_WEB_HASH_VALUE_H
