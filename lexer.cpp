
#include "msl.h"

namespace MSL
{
    static inline bool IsDecimalNumber(char ch)
    {
        return ((ch >= '0') && (ch <= '9'));
    }
    static inline bool IsHexadecimalNumber(char ch)
    {
        return (
            ((ch >= '0') && (ch <= '9')) ||
            ((ch >= 'A') && (ch <= 'F')) ||
            ((ch >= 'a') && (ch <= 'f'))
        );
    }
    static inline bool IsAlpha(char ch)
    {
        return (
            ((ch >= 'a') && (ch <= 'z')) ||
            ((ch >= 'A') && (ch <= 'Z')) ||
            (ch == '_')
        );
    }
    static inline bool IsAlphaNumeric(char ch)
    {
        return (
            ((ch >= 'a') && (ch <= 'z')) ||
            ((ch >= 'A') && (ch <= 'Z')) ||
            ((ch >= '0') && (ch <= '9')) ||
            (ch == '_')
        );
    }

    CodeReader::CodeReader(std::string_view code) : m_code{ code }, m_place{ 0, 1, 1 }
    {
    }

    bool CodeReader::isAtEnd() const
    {
        return m_place.textindex >= m_code.length();
    }

    const Location& CodeReader::getCurrentPlace() const
    {
        return m_place;
    }

    const char* CodeReader::getCurrentCode() const
    {
        return m_code.data() + m_place.textindex;
    }

    size_t CodeReader::getCurrentLength() const
    {
        return m_code.length() - m_place.textindex;
    }

    char CodeReader::getCurrentChar() const
    {
        return m_code[m_place.textindex];
    }

    bool CodeReader::peekNext(char ch) const
    {
        return m_place.textindex < m_code.length() && m_code[m_place.textindex] == ch;
    }

    bool CodeReader::peekNext(const char* s, size_t sLen) const
    {
        return m_place.textindex + sLen <= m_code.length() && memcmp(m_code.data() + m_place.textindex, s, sLen) == 0;
    }

    void CodeReader::moveOneChar()
    {
        if(m_code[m_place.textindex++] == '\n')
        {
            m_place.textline++;
            m_place.textcolumn = 1;
        }
        else
        {
            ++m_place.textcolumn;
        }
    }

    void CodeReader::MoveChars(size_t n)
    {
        for(size_t i = 0; i < n; ++i)
            moveOneChar();
    }

    bool Tokenizer::parseHexChar(uint8_t& out, char ch)
    {
        if(ch >= '0' && ch <= '9')
            out = (uint8_t)(ch - '0');
        else if(ch >= 'a' && ch <= 'f')
            out = (uint8_t)(ch - 'a' + 10);
        else if(ch >= 'A' && ch <= 'F')
            out = (uint8_t)(ch - 'A' + 10);
        else
            return false;
        return true;
    }

    bool Tokenizer::parseHexLiteral(uint32_t& out, std::string_view chars)
    {
        size_t i;
        size_t count;
        out = 0;
        for(i = 0, count = chars.length(); i < count; ++i)
        {
            uint8_t charVal = 0;
            if(!parseHexChar(charVal, chars[i]))
                return false;
            out = (out << 4) | charVal;
        }
        return true;
    }

    bool Tokenizer::appendUTF8Char(std::string& inout, uint32_t charVal)
    {
        if(charVal <= 0x7F)
            inout += (char)(uint8_t)charVal;
        else if(charVal <= 0x7FF)
        {
            inout += (char)(uint8_t)(0b11000000 | (charVal >> 6));
            inout += (char)(uint8_t)(0b10000000 | (charVal & 0b111111));
        }
        else if(charVal <= 0xFFFF)
        {
            inout += (char)(uint8_t)(0b11100000 | (charVal >> 12));
            inout += (char)(uint8_t)(0b10000000 | ((charVal >> 6) & 0b111111));
            inout += (char)(uint8_t)(0b10000000 | (charVal & 0b111111));
        }
        else if(charVal <= 0x10FFFF)
        {
            inout += (char)(uint8_t)(0b11110000 | (charVal >> 18));
            inout += (char)(uint8_t)(0b10000000 | ((charVal >> 12) & 0b111111));
            inout += (char)(uint8_t)(0b10000000 | ((charVal >> 6) & 0b111111));
            inout += (char)(uint8_t)(0b10000000 | (charVal & 0b111111));
        }
        else
            return false;
        return true;
    }

    void Tokenizer::skipSpacesAndComments()
    {
        while(!m_code.isAtEnd())
        {
            // Whitespace
            if(isspace(m_code.getCurrentChar()))
                m_code.moveOneChar();
            // Single line comment
            else if(m_code.peekNext("//", 2))
            {
                m_code.MoveChars(2);
                while(!m_code.isAtEnd() && m_code.getCurrentChar() != '\n')
                    m_code.moveOneChar();
            }
            // Multi line comment
            else if(m_code.peekNext("/*", 2))
            {
                for(m_code.MoveChars(2);; m_code.moveOneChar())
                {
                    if(m_code.isAtEnd())
                        throw Error::ParsingError(m_code.getCurrentPlace(), "unexpected EOF while looking for end of comment block");
                    else if(m_code.peekNext("*/", 2))
                    {
                        m_code.MoveChars(2);
                        break;
                    }
                }
            }
            else
                break;
        }
    }

    bool Tokenizer::parseNumber(Token& out)
    {
        enum { kBufSize = 128 };
        uint64_t n;
        uint64_t i;
        uint8_t val;
        size_t tokenLen;
        size_t currentCodeLen;
        const char* currentCode;
        char sz[kBufSize + 1];

        currentCode = m_code.getCurrentCode();
        currentCodeLen = m_code.getCurrentLength();
        if(!IsDecimalNumber(currentCode[0]) && currentCode[0] != '.')
        {
            return false;
        }
        tokenLen = 0;
        // Hexadecimal: 0xHHHH...
        if(currentCode[0] == '0' && currentCodeLen >= 2 && (currentCode[1] == 'x' || currentCode[1] == 'X'))
        {
            tokenLen = 2;
            while(tokenLen < currentCodeLen && IsHexadecimalNumber(currentCode[tokenLen]))
                ++tokenLen;
            if(tokenLen < 3)
                throw Error::ParsingError(out.m_place, "invalid number");
            n = 0;
            for(i = 2; i < tokenLen; ++i)
            {
                val = 0;
                parseHexChar(val, currentCode[i]);
                n = (n << 4) | val;
            }
            out.numberval = (double)n;
        }
        else
        {
            while(tokenLen < currentCodeLen && IsDecimalNumber(currentCode[tokenLen]))
                ++tokenLen;
            const size_t digitsBeforeDecimalPoint = tokenLen;
            size_t digitsAfterDecimalPoint = 0;
            if(tokenLen < currentCodeLen && currentCode[tokenLen] == '.')
            {
                ++tokenLen;
                while(tokenLen < currentCodeLen && IsDecimalNumber(currentCode[tokenLen]))
                    ++tokenLen;
                digitsAfterDecimalPoint = tokenLen - digitsBeforeDecimalPoint - 1;
            }
            // Only dot '.' with no digits around: not a number token.
            if(digitsBeforeDecimalPoint + digitsAfterDecimalPoint == 0)
                return false;
            if(tokenLen < currentCodeLen && (currentCode[tokenLen] == 'e' || currentCode[tokenLen] == 'E'))
            {
                ++tokenLen;
                if(tokenLen < currentCodeLen && (currentCode[tokenLen] == '+' || currentCode[tokenLen] == '-'))
                    ++tokenLen;
                const size_t tokenLenBeforeExponent = tokenLen;
                while(tokenLen < currentCodeLen && IsDecimalNumber(currentCode[tokenLen]))
                    ++tokenLen;
                if(tokenLen - tokenLenBeforeExponent == 0)
                    throw Error::ParsingError(out.m_place, "invalid number: bad exponent");
            }
            if(tokenLen >= kBufSize)
            {
                throw Error::ParsingError(out.m_place, "invalid number");
            }
            memcpy(sz, currentCode, tokenLen);
            sz[tokenLen] = 0;
            out.numberval = atof(sz);
        }
        // Letters straight after number are invalid.
        if(tokenLen < currentCodeLen && IsAlpha(currentCode[tokenLen]))
            throw Error::ParsingError(out.m_place, "invalid number followed by alpha letters");
        out.symtype = Token::Type::Number;
        m_code.MoveChars(tokenLen);
        return true;
    }

    bool Tokenizer::parseString(Token& out)
    {
        size_t tokenLen;
        size_t currCodeLen;
        char delimiterCh;
        const char* currCode;
        currCode = m_code.getCurrentCode();
        currCodeLen = m_code.getCurrentLength();
        delimiterCh = currCode[0];
        if(delimiterCh != '"' && delimiterCh != '\'')
        {
            return false;
        }
        out.symtype = Token::Type::String;
        out.stringval.clear();
        tokenLen = 1;
        for(;;)
        {
            if(tokenLen == currCodeLen)
                throw Error::ParsingError(out.m_place, "unexpected EOF while parsing string");
            if(currCode[tokenLen] == delimiterCh)
                break;
            if(currCode[tokenLen] == '\\')
            {
                ++tokenLen;
                if(tokenLen == currCodeLen)
                    throw Error::ParsingError(out.m_place, "unexpected EOF while parsing string");
                switch(currCode[tokenLen])
                {
                    case '\\':
                        out.stringval += '\\';
                        ++tokenLen;
                        break;
                    case '/':
                        out.stringval += '/';
                        ++tokenLen;
                        break;
                    case '"':
                        out.stringval += '"';
                        ++tokenLen;
                        break;
                    case '\'':
                        out.stringval += '\'';
                        ++tokenLen;
                        break;
                    case '?':
                        out.stringval += '?';
                        ++tokenLen;
                        break;
                    case 'a':
                        out.stringval += '\a';
                        ++tokenLen;
                        break;
                    case 'b':
                        out.stringval += '\b';
                        ++tokenLen;
                        break;
                    case 'f':
                        out.stringval += '\f';
                        ++tokenLen;
                        break;
                    case 'n':
                        out.stringval += '\n';
                        ++tokenLen;
                        break;
                    case 'r':
                        out.stringval += '\r';
                        ++tokenLen;
                        break;
                    case 't':
                        out.stringval += '\t';
                        ++tokenLen;
                        break;
                    case 'v':
                        out.stringval += '\v';
                        ++tokenLen;
                        break;
                    case '0':
                        out.stringval += '\0';
                        ++tokenLen;
                        break;
                    case 'x':
                    {
                        uint32_t val = 0;
                        if(tokenLen + 2 >= currCodeLen || !parseHexLiteral(val, std::string_view{ currCode + tokenLen + 1, 2 }))
                            throw Error::ParsingError(out.m_place, "invalid string escape sequence: '\\x' too short");
                        out.stringval += (char)(uint8_t)val;
                        tokenLen += 3;
                        break;
                    }
                    case 'u':
                    {
                        uint32_t val = 0;
                        if(tokenLen + 4 >= currCodeLen || !parseHexLiteral(val, std::string_view{ currCode + tokenLen + 1, 4 }))
                            throw Error::ParsingError(out.m_place, "invalid string escape sequence: unicode too short");
                        if(!appendUTF8Char(out.stringval, val))
                            throw Error::ParsingError(out.m_place, "invalid string escape sequence: invalid unicode");
                        tokenLen += 5;
                        break;
                    }
                    case 'U':
                    {
                        uint32_t val = 0;
                        if(tokenLen + 8 >= currCodeLen || !parseHexLiteral(val, std::string_view{ currCode + tokenLen + 1, 8 }))
                            throw Error::ParsingError(out.m_place, "invalid string escape sequence: bad hexadecimal");
                        if(!appendUTF8Char(out.stringval, val))
                            throw Error::ParsingError(out.m_place,"invalid string escape sequence: bad unicode");
                        tokenLen += 9;
                        break;
                    }
                    default:
                        throw Error::ParsingError(out.m_place, "invalid string escape sequence: unknown escape sequence");
                }
            }
            else
                out.stringval += currCode[tokenLen++];
        }
        ++tokenLen;
        // Letters straight after string are invalid.
        if(tokenLen < currCodeLen && IsAlpha(currCode[tokenLen]))
            throw Error::ParsingError(out.m_place, "invalid string followed by letters");
        m_code.MoveChars(tokenLen);
        return true;
    }

    void Tokenizer::getNextToken(Token& out)
    {
        skipSpacesAndComments();

        out.m_place = m_code.getCurrentPlace();

        // End of input
        if(m_code.isAtEnd())
        {
            out.symtype = Token::Type::End;
            return;
        }

        constexpr Token::Type firstSingleCharSymbol = Token::Type::Comma;
        constexpr Token::Type firstMultiCharSymbol = Token::Type::DoublePlus;
        constexpr Token::Type firstKeywordSymbol = Token::Type::Null;

        const char* const currentCode = m_code.getCurrentCode();
        const size_t currentCodeLen = m_code.getCurrentLength();

        if(parseString(out))
            return;
        if(parseNumber(out))
            return;
        // Multi char symbol
        for(size_t i = (size_t)firstMultiCharSymbol; i < (size_t)firstKeywordSymbol; ++i)
        {
            const size_t symbolLen = SYMBOL_STR[i].length();
            if(currentCodeLen >= symbolLen && memcmp(SYMBOL_STR[i].data(), currentCode, symbolLen) == 0)
            {
                out.symtype = (Token::Type)i;
                m_code.MoveChars(symbolLen);
                return;
            }
        }
        // symtype
        for(size_t i = (size_t)firstSingleCharSymbol; i < (size_t)firstMultiCharSymbol; ++i)
        {
            if(currentCode[0] == SYMBOL_STR[i][0])
            {
                out.symtype = (Token::Type)i;
                m_code.moveOneChar();
                return;
            }
        }
        // Identifier or keyword
        if(IsAlpha(currentCode[0]))
        {
            size_t tokenLen = 1;
            while(tokenLen < currentCodeLen && IsAlphaNumeric(currentCode[tokenLen]))
                ++tokenLen;
            // Keyword
            for(size_t i = (size_t)firstKeywordSymbol; i < (size_t)Token::Type::Count; ++i)
            {
                const size_t keywordLen = SYMBOL_STR[i].length();
                if(keywordLen == tokenLen && memcmp(SYMBOL_STR[i].data(), currentCode, tokenLen) == 0)
                {
                    out.symtype = (Token::Type)i;
                    m_code.MoveChars(keywordLen);
                    return;
                }
            }
            // Identifier
            out.symtype = Token::Type::Identifier;
            out.stringval = std::string{ currentCode, currentCode + tokenLen };
            m_code.MoveChars(tokenLen);
            return;
        }
        throw Error::ParsingError(out.m_place, "unexpected token");
    }
}

