
#include "msl.h"

namespace MSL
{
    namespace Util
    {
        std::shared_ptr<Object> CopyObject(const Object& src)
        {
            auto dst = std::make_shared<Object>();
            for(const auto& item : src.m_entrymap)
                dst->m_entrymap.insert(item);
            return dst;
        }

        std::shared_ptr<Array> CopyArray(const Array& src)
        {
            auto dst = std::make_shared<Array>();
            const size_t count = src.size();
            dst->resize(count);
            for(size_t i = 0; i < count; ++i)
                dst->at(i) = Value{ src.at(i) };
            return dst;
        }

        bool NumberToIndex(size_t& outIndex, double number)
        {
            if(!std::isfinite(number) || number < 0.f)
            {
                return false;
            }
            outIndex = (size_t)number;
            return ((double)outIndex == number);
        }

        void reprChar(std::ostream& os, int c)
        {
            switch(c)
            {
                case '\'':
                    os << "\\\'";
                    break;
                case '\"':
                    os << "\\\"";
                    break;
                case '\\':
                    os << "\\\\";
                    break;
                case '\b':
                    os << "\\b";
                    break;
                case '\f':
                    os << "\\f";
                    break;
                case '\n':
                    os << "\\n";
                    break;
                case '\r':
                    os << "\\r";
                    break;
                case '\t':
                    os << "\\t";
                    break;
                default:
                    {
                        constexpr const char* const hexchars = "0123456789ABCDEF";
                        os << '\\';
                        if(c <= 255)
                        {
                            os << 'x';
                            os << hexchars[(c >> 4) & 0xf];
                            os << hexchars[c & 0xf];
                        }
                        else
                        {
                            os << 'u';
                            os << hexchars[(c >> 12) & 0xf];
                            os << hexchars[(c >> 8) & 0xf];
                            os << hexchars[(c >> 4) & 0xf];
                            os << hexchars[c & 0xf];
                        }
                    }
                    break;
            }
        }

        void reprString(std::ostream& os, std::string_view str)
        {
            int ch;
            size_t i;
            os << '"';
            for(i=0; i<str.size(); i++)
            {
                ch = str[i];
                if((ch < 32) || (ch > 127) || (ch == '\"') || (ch == '\\'))
                {
                    reprChar(os, ch);
                }
                else
                {
                    os.put(char(ch));
                }
            }
            os << '"';
        }

        std::string vformatString(const char* format, va_list argList)
        {
            size_t dstlen;
            size_t rtlen;
            dstlen = 1024*1024;
            std::vector<char> buf(dstlen + 1);
            rtlen = vsnprintf(&buf[0], dstlen + 1, format, argList);
            return std::string{ &buf[0], rtlen };
        }

        std::string formatString(const char* format, ...)
        {
            va_list argList;
            va_start(argList, format);
            auto result = vformatString(format, argList);
            va_end(argList);
            return result;
        }

        void stripInplaceLeft(std::string& str)
        {
            str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch)
            {
                return !std::isspace(ch);
            }));
        }

        void stripInplaceRight(std::string& str)
        {
            str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch)
            {
                return !std::isspace(ch);
            }).base(), str.end());
        }

        void stripInplace(std::string& str)
        {
            stripInplaceLeft(str);
            stripInplaceRight(str);
        }

        std::string strip(std::string_view in)
        {
            std::string copy(in);
            stripInplace(copy);
            return copy;
        }

        std::string stripRight(std::string_view s)
        {
            std::string copy(s);
            stripInplaceRight(copy);
            return copy;
        }

        std::string stripLeft(std::string_view s)
        {
            std::string copy(s);
            stripInplaceLeft(copy);
            return copy;
        }

        void splitString(std::string_view str, const std::string& delim, std::function<bool(const std::string&)> cb)
        {
            size_t prevpos;
            size_t herepos;
            std::string token;
            prevpos = 0;
            herepos = 0;
            while(true)
            {
                herepos = str.find(delim, prevpos);
                if(herepos == std::string::npos)
                {
                    herepos = str.size();
                }
                token = str.substr(prevpos, herepos-prevpos);
                stripInplace(token);
                if (!token.empty())
                {
                    if(!cb(token))
                    {
                        return;
                    }
                }
                prevpos = herepos + delim.size();
                if((herepos > str.size()) || (prevpos > str.size()))
                {
                    break;
                }
            }
        }

        void checkArgumentCount(const Location& place, std::string_view fname, size_t argcnt, size_t expect)
        {
            if((expect > 0) && ((argcnt == 0) || (argcnt < expect)))
            {
                throw Error::ArgumentError(place, Util::joinArgs("function '", fname, "' expects at least ", expect, " arguments"));
            }
        }

        Value checkArgument(const Location& place, std::string_view name, const Value::List& args, size_t idx, Value::Type type)
        {
            Value r;
            checkArgumentCount(place, name, args.size(), idx);
            r = args[idx];
            // using Value::Type::Null means any type
            if((type != Value::Type::Null) && (r.type() != type))
            {
                auto etype = Value::getTypename(type);
                auto gtype = Value::getTypename(r.type());
                throw Error::TypeError(place,
                    Util::joinArgs("expected argument #", idx, " to be a ", etype, ", but got ", gtype, " instead"));
            }
            return r;
        }

    }
}

