
#include "msl.h"

namespace MSL
{
    struct SymTypePair
    {
        Token::Type type;
        std::string_view text;
    };

    static constexpr auto g_symboltable = std::to_array<SymTypePair>(
    {
        // blanko symbol
        { Token::Type::None, ""},

        // type "identifiers" (i.e., String(), Array(), ...)
        // these *must* be empty, as they're identified by type.
        { Token::Type::Identifier, ""},
        { Token::Type::Number, ""},
        { Token::Type::String, ""},
        { Token::Type::End, ""},

        // single character operators
        { Token::Type::Comma, ","},
        { Token::Type::QuestionMark, "?"},
        { Token::Type::Colon, ":"},
        { Token::Type::Semicolon, ";"},
        { Token::Type::RoundBracketOpen, "("},
        { Token::Type::RoundBracketClose, ")"},
        { Token::Type::SquareBracketOpen, "["},
        { Token::Type::SquareBracketClose, "]"},
        { Token::Type::CurlyBracketOpen, "{"},
        { Token::Type::CurlyBracketClose, "}"},
        { Token::Type::Asterisk, "*"},
        { Token::Type::Slash, "/"},
        { Token::Type::Percent, "%"},
        { Token::Type::Plus, "+"},
        { Token::Type::Dash, "-"},
        { Token::Type::Equals, "="},
        { Token::Type::ExclamationMark, "!"},
        { Token::Type::Tilde, "~"},
        { Token::Type::Less, "<"},
        { Token::Type::Greater, ">"},
        { Token::Type::Amperstand, "&"},
        { Token::Type::Caret, "^"},
        { Token::Type::Pipe, "|"},
        { Token::Type::Dot, "."},

        // multi-character operators
        { Token::Type::DoublePlus, "++"},
        { Token::Type::DoubleDash, "--"},
        { Token::Type::PlusEquals, "+="},
        { Token::Type::DashEquals, "-="},
        { Token::Type::AsteriskEquals, "*="},
        { Token::Type::SlashEquals, "/="},
        { Token::Type::PercentEquals, "%="},
        { Token::Type::DoubleLessEquals, "<<="},
        { Token::Type::DoubleGreaterEquals, ">>="},
        { Token::Type::AmperstandEquals, "&="},
        { Token::Type::CaretEquals, "^="},
        { Token::Type::PipeEquals, "|="},
        { Token::Type::DoubleLess, "<<"},
        { Token::Type::DoubleGreater, ">>"},
        { Token::Type::LessEquals, "<="},
        { Token::Type::GreaterEquals, ">="},
        { Token::Type::DoubleEquals, "=="},
        { Token::Type::ExclamationEquals, "!="},
        { Token::Type::DoubleAmperstand, "&&"},
        { Token::Type::DoublePipe, "||"},

        // keywords
        { Token::Type::Null, "null"},
        { Token::Type::False, "false"},
        { Token::Type::True, "true"},
        { Token::Type::If, "if"},
        { Token::Type::Else, "else"},
        { Token::Type::While, "while"},
        { Token::Type::Do, "do"},
        { Token::Type::For, "for"},
        { Token::Type::Break, "break"},
        { Token::Type::Continue, "continue"},
        { Token::Type::Switch, "switch"},
        { Token::Type::Case, "case"},
        { Token::Type::Default, "default"},
        { Token::Type::Function, "function"},
        { Token::Type::Return, "return"},
        { Token::Type::Local, "local"},
        { Token::Type::This, "this"},
        { Token::Type::Global, "global"},
        { Token::Type::Class, "class"},
        { Token::Type::Throw, "throw"},
        { Token::Type::Try, "try"},
        { Token::Type::Catch, "catch"},
        { Token::Type::Finally, "finally"},
    });

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
            (ch == '_') ||
            (ch == '$')
        );
    }

    CodeReader::CodeReader(std::string_view code) : m_code{ code }, m_place{ 0, 1, 1 }
    {
    }

    bool CodeReader::isAtEnd() const
    {
        return m_place.textindex >= m_code.length();
    }

    const Location& CodeReader::location() const
    {
        return m_place;
    }

    const char* CodeReader::code() const
    {
        return m_code.data() + m_place.textindex;
    }

    size_t CodeReader::codeLength() const
    {
        return m_code.length() - m_place.textindex;
    }

    char CodeReader::getCurrent() const
    {
        return m_code[m_place.textindex];
    }

    bool CodeReader::peekNext(char ch) const
    {
        return (
            (m_place.textindex < m_code.length()) &&
            (m_code[m_place.textindex] == ch)
        );
    }

    bool CodeReader::peekNext(const char* s, size_t slen) const
    {
        return (
            (m_place.textindex + slen <= m_code.length()) &&
            (memcmp(m_code.data() + m_place.textindex, s, slen) == 0)
        );
    }

    void CodeReader::moveForward()
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

    void CodeReader::moveForward(size_t n)
    {
        size_t i;
        for(i = 0; i < n; ++i)
        {
            moveForward();
        }
    }

    bool Tokenizer::parseHexChar(uint8_t& out, char ch)
    {
        if((ch >= '0') && (ch <= '9'))
        {
            out = (uint8_t)(ch - '0');
        }
        else if((ch >= 'a') && (ch <= 'f'))
        {
            out = (uint8_t)(ch - 'a' + 10);
        }
        else if((ch >= 'A') && (ch <= 'F'))
        {
            out = (uint8_t)(ch - 'A' + 10);
        }
        else
        {
            return false;
        }
        return true;
    }

    bool Tokenizer::parseHexLiteral(uint32_t& out, std::string_view chars)
    {
        size_t i;
        size_t count;
        uint8_t chv;
        out = 0;
        for(i = 0, count = chars.length(); i < count; ++i)
        {
            chv = 0;
            if(!parseHexChar(chv, chars[i]))
            {
                return false;
            }
            out = (out << 4) | chv;
        }
        return true;
    }

    bool Tokenizer::appendUTF8Char(std::string& inout, uint32_t chv)
    {
        if(chv <= 0x7F)
        {
            inout += (char)(uint8_t)chv;
        }
        else if(chv <= 0x7FF)
        {
            inout += (char)(uint8_t)(0b11000000 | (chv >> 6));
            inout += (char)(uint8_t)(0b10000000 | (chv & 0b111111));
        }
        else if(chv <= 0xFFFF)
        {
            inout += (char)(uint8_t)(0b11100000 | (chv >> 12));
            inout += (char)(uint8_t)(0b10000000 | ((chv >> 6) & 0b111111));
            inout += (char)(uint8_t)(0b10000000 | (chv & 0b111111));
        }
        else if(chv <= 0x10FFFF)
        {
            inout += (char)(uint8_t)(0b11110000 | (chv >> 18));
            inout += (char)(uint8_t)(0b10000000 | ((chv >> 12) & 0b111111));
            inout += (char)(uint8_t)(0b10000000 | ((chv >> 6) & 0b111111));
            inout += (char)(uint8_t)(0b10000000 | (chv & 0b111111));
        }
        else
        {
            return false;
        }
        return true;
    }

    void Tokenizer::skipSpacesAndComments()
    {
        while(!m_code.isAtEnd())
        {
            // Whitespace
            if(isspace(m_code.getCurrent()))
            {
                m_code.moveForward();
            }
            // Single line comment
            else if(m_code.peekNext("//", 2))
            {
                m_code.moveForward(2);
                while(!m_code.isAtEnd() && m_code.getCurrent() != '\n')
                {
                    m_code.moveForward();
                }
            }
            // Multi line comment
            else if(m_code.peekNext("/*", 2))
            {
                for(m_code.moveForward(2);; m_code.moveForward())
                {
                    if(m_code.isAtEnd())
                    {
                        throw Error::ParsingError(m_code.location(), "unexpected EOF while looking for end of comment block");
                    }
                    else if(m_code.peekNext("*/", 2))
                    {
                        m_code.moveForward(2);
                        break;
                    }
                }
            }
            else
            {
                break;
            }
        }
    }

    bool Tokenizer::parseNumber(Token& out)
    {
        enum { kBufSize = 128 };
        uint64_t n;
        uint64_t i;
        uint8_t val;
        size_t tklen;
        size_t currlen;
        size_t digitsbeforepoint;
        size_t tklenbeforeexp;
        size_t digitsafterpoint;
        const char* currcode;
        char sz[kBufSize + 1];
        currcode = m_code.code();
        currlen = m_code.codeLength();
        if(!IsDecimalNumber(currcode[0]) && currcode[0] != '.')
        {
            return false;
        }
        tklen = 0;
        // Hexadecimal: 0xHHHH...
        if(currcode[0] == '0' && currlen >= 2 && (currcode[1] == 'x' || currcode[1] == 'X'))
        {
            tklen = 2;
            while(tklen < currlen && IsHexadecimalNumber(currcode[tklen]))
            {
                ++tklen;
            }
            if(tklen < 3)
            {
                throw Error::ParsingError(out.m_place, "invalid number");
            }
            n = 0;
            for(i = 2; i < tklen; ++i)
            {
                val = 0;
                parseHexChar(val, currcode[i]);
                n = (n << 4) | val;
            }
            out.number() = (double)n;
        }
        else
        {
            while(tklen < currlen && IsDecimalNumber(currcode[tklen]))
            {
                ++tklen;
            }
            digitsbeforepoint = tklen;
            digitsafterpoint = 0;
            if(tklen < currlen && currcode[tklen] == '.')
            {
                ++tklen;
                while(tklen < currlen && IsDecimalNumber(currcode[tklen]))
                {
                    ++tklen;
                }
                digitsafterpoint = tklen - digitsbeforepoint - 1;
            }
            // Only dot '.' with no digits around: not a number token.
            if(digitsbeforepoint + digitsafterpoint == 0)
            {
                return false;
            }
            if(tklen < currlen && (currcode[tklen] == 'e' || currcode[tklen] == 'E'))
            {
                ++tklen;
                if(tklen < currlen && (currcode[tklen] == '+' || currcode[tklen] == '-'))
                {
                    ++tklen;
                }
                tklenbeforeexp = tklen;
                while(tklen < currlen && IsDecimalNumber(currcode[tklen]))
                {
                    ++tklen;
                }
                if(tklen - tklenbeforeexp == 0)
                {
                    throw Error::ParsingError(out.m_place, "invalid number: bad exponent");
                }
            }
            if(tklen >= kBufSize)
            {
                throw Error::ParsingError(out.m_place, "invalid number");
            }
            memcpy(sz, currcode, tklen);
            sz[tklen] = 0;
            out.number() = atof(sz);
        }
        // Letters straight after number are invalid.
        if(tklen < currlen && IsAlpha(currcode[tklen]))
        {
            throw Error::ParsingError(out.m_place, "invalid number followed by alpha letters");
        }
        out.type(Token::Type::Number);
        m_code.moveForward(tklen);
        return true;
    }

    bool Tokenizer::parseString(Token& out)
    {
        size_t tklen;
        size_t currlen;
        uint32_t val;
        char chdelim;
        const char* currcode;
        currcode = m_code.code();
        currlen = m_code.codeLength();
        chdelim = currcode[0];
        if(chdelim != '"' && chdelim != '\'')
        {
            return false;
        }
        out.type(Token::Type::String);
        out.string().clear();
        tklen = 1;
        for(;;)
        {
            if(tklen == currlen)
            {
                throw Error::ParsingError(out.m_place, "unexpected EOF while parsing string");
            }
            if(currcode[tklen] == chdelim)
            {
                break;
            }
            if(currcode[tklen] == '\\')
            {
                ++tklen;
                if(tklen == currlen)
                {
                    throw Error::ParsingError(out.m_place, "unexpected EOF while parsing string");
                }
                switch(currcode[tklen])
                {
                    case '\\':
                        {
                            out.string() += '\\';
                            ++tklen;
                        }
                        break;
                    case '/':
                        {
                            out.string() += '/';
                            ++tklen;
                        }
                        break;
                    case '"':
                        {
                            out.string() += '"';
                            ++tklen;
                        }
                        break;
                    case '\'':
                        {
                            out.string() += '\'';
                            ++tklen;
                        }
                        break;
                    case '?':
                        {
                            out.string() += '?';
                            ++tklen;
                        }
                        break;
                    case 'a':
                        {
                            out.string() += '\a';
                            ++tklen;
                        }
                        break;
                    case 'b':
                        {
                            out.string() += '\b';
                            ++tklen;
                        }
                        break;
                    case 'f':
                        {
                            out.string() += '\f';
                            ++tklen;
                        }
                        break;
                    case 'n':
                        {
                            out.string() += '\n';
                            ++tklen;
                        }
                        break;
                    case 'r':
                        {
                            out.string() += '\r';
                            ++tklen;
                        }
                        break;
                    case 't':
                        {
                            out.string() += '\t';
                            ++tklen;
                        }
                        break;
                    case 'v':
                        {
                            out.string() += '\v';
                            ++tklen;
                        }
                        break;
                    case '0':
                        {
                            out.string() += '\0';
                            ++tklen;
                        }
                        break;
                    case 'x':
                        {
                            val = 0;
                            if(tklen + 2 >= currlen || !parseHexLiteral(val, std::string_view{ currcode + tklen + 1, 2 }))
                            {
                                throw Error::ParsingError(out.m_place, "invalid string escape sequence: '\\x' too short");
                            }
                            out.string() += (char)(uint8_t)val;
                            tklen += 3;
                        }
                        break;
                    case 'u':
                        {
                            val = 0;
                            if(tklen + 4 >= currlen || !parseHexLiteral(val, std::string_view{ currcode + tklen + 1, 4 }))
                            {
                                throw Error::ParsingError(out.m_place, "invalid string escape sequence: unicode too short");
                            }
                            if(!appendUTF8Char(out.string(), val))
                            {
                                throw Error::ParsingError(out.m_place, "invalid string escape sequence: invalid unicode");
                            }
                            tklen += 5;
                        }
                        break;
                    case 'U':
                        {
                            val = 0;
                            if(tklen + 8 >= currlen || !parseHexLiteral(val, std::string_view{ currcode + tklen + 1, 8 }))
                            {
                                throw Error::ParsingError(out.m_place, "invalid string escape sequence: bad hexadecimal");
                            }
                            if(!appendUTF8Char(out.string(), val))
                            {
                                throw Error::ParsingError(out.m_place,"invalid string escape sequence: bad unicode");
                            }
                            tklen += 9;
                        }
                        break;
                    default:
                        {
                            throw Error::ParsingError(out.m_place, "invalid string escape sequence: unknown escape sequence");
                        }
                        break;
                }
            }
            else
            {
                out.string() += currcode[tklen++];
            }
        }
        ++tklen;
        // Letters straight after string are invalid.
        if(tklen < currlen && IsAlpha(currcode[tklen]))
        {
            throw Error::ParsingError(out.m_place, "invalid string followed by letters");
        }
        m_code.moveForward(tklen);
        return true;
    }

    void Tokenizer::getNextToken(Token& out)
    {
        size_t i;
        size_t tklen;
        size_t kwlen;
        size_t symlen;
        size_t currlen;
        const char*  currcode;
        constexpr Token::Type firstsinglecharsym = Token::Type::Comma;
        constexpr Token::Type firstmulticharsym = Token::Type::DoublePlus;
        constexpr Token::Type firstkwsym = Token::Type::Null;
        skipSpacesAndComments();
        out.m_place = m_code.location();
        // End of input
        if(m_code.isAtEnd())
        {
            out.type(Token::Type::End);
            return;
        }
        currcode = m_code.code();
        currlen = m_code.codeLength();
        if(parseString(out))
        {
            return;
        }
        if(parseNumber(out))
        {
            return;
        }
        // Multi char symbol
        for(i = (size_t)firstmulticharsym; i < (size_t)firstkwsym; ++i)
        {
            symlen = g_symboltable[i].text.length();
            if(currlen >= symlen && memcmp(g_symboltable[i].text.data(), currcode, symlen) == 0)
            {
                out.type(g_symboltable[i].type);
                m_code.moveForward(symlen);
                return;
            }
        }
        // symtype
        for(i = (size_t)firstsinglecharsym; i < (size_t)firstmulticharsym; ++i)
        {
            if(currcode[0] == g_symboltable[i].text[0])
            {
                out.type(g_symboltable[i].type);
                m_code.moveForward();
                return;
            }
        }
        // Identifier or keyword
        if(IsAlpha(currcode[0]) || (currcode[0] == '$'))
        {
            tklen = 1;
            while(tklen < currlen && IsAlphaNumeric(currcode[tklen]))
            {
                ++tklen;
            }
            // Keyword
            for(i = (size_t)firstkwsym; i < (size_t)Token::Type::Count; ++i)
            {
                kwlen = g_symboltable[i].text.length();
                if(kwlen == tklen && memcmp(g_symboltable[i].text.data(), currcode, tklen) == 0)
                {
                    out.type(g_symboltable[i].type);
                    m_code.moveForward(kwlen);
                    return;
                }
            }
            // Identifier
            out.type(Token::Type::Identifier);
            out.string() = std::string{ currcode, currcode + tklen };
            m_code.moveForward(tklen);
            return;
        }
        throw Error::ParsingError(out.m_place, "unexpected token");
    }
}

