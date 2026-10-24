#pragma once

#include <stdint.h>

// Append a JSON string value, including its surrounding quotes.
// StringLike must support operator+=(char) and operator+=(const char *).
template <typename StringLike>
void appendJsonString(StringLike &json, const char *value)
{
    static const char JSON_HEX_DIGITS[] = "0123456789ABCDEF";
    json += '"';

    if (value != nullptr)
    {
        const unsigned char *cursor =
            reinterpret_cast<const unsigned char *>(value);
        while (*cursor != '\0')
        {
            const unsigned char ch = *cursor++;
            switch (ch)
            {
            case '"': json += "\\\""; break;
            case '\\': json += "\\\\"; break;
            case '\b': json += "\\b"; break;
            case '\f': json += "\\f"; break;
            case '\n': json += "\\n"; break;
            case '\r': json += "\\r"; break;
            case '\t': json += "\\t"; break;
            default:
                if (ch < 0x20)
                {
                    json += "\\u00";
                    json += JSON_HEX_DIGITS[(ch >> 4) & 0x0F];
                    json += JSON_HEX_DIGITS[ch & 0x0F];
                }
                else
                {
                    json += static_cast<char>(ch);
                }
                break;
            }
        }
    }

    json += '"';
}
