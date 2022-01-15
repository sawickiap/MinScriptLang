
#include <sstream>
#include "msl.h"

namespace MSL
{
    namespace
    {
        template<typename CharT>
        void cppquote_char(std::basic_ostream<CharT>& os, int c)
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

        template<typename CharT>        
        void cppquote_string(std::basic_ostream<CharT>& os, std::string_view str)
        {
            int ch;
            size_t i;
            os << '"';
            for(i=0; i<str.size(); i++)
            {
                ch = str[i];
                if((ch < 32) || (ch > 127) || (ch == '\"') || (ch == '\\'))
                {
                    cppquote_char(os, ch);
                }
                else
                {
                    os.put(char(ch));
                }
            }
            os << '"';
        }

        template<typename CharT>
        void dump_array(std::basic_ostream<CharT>& os, Array* arr, bool repr)
        {
            size_t i;
            size_t sz;
            sz = arr->size();
            os << "[";
            for(i=0; i<sz; i++)
            {
                auto val = arr->at(i);
                if(val.isArray() && (val.getArray() == arr))
                {
                    os << "<recursion>";
                }
                else
                {
                    val.toStream(os, repr);
                }
                if((i+1) != sz)
                {
                    os << ", ";
                }
            }
            os << "]";
        }
    }

    bool Value::isEqual(const Value& rhs) const
    {
        if(m_type != rhs.m_type)
            return false;
        switch(m_type)
        {
            case Value::Type::Null:
                {
                    return true;
                }
            case Value::Type::Number:
                {
                    return std::get<NumberValType>(m_variant) == std::get<NumberValType>(rhs.m_variant);
                }
            case Value::Type::String:
                {
                    return std::get<StringValType>(m_variant) == std::get<StringValType>(rhs.m_variant);
                }
            #if 0
            case Value::Type::Function:
                {
                    return std::get<AstFuncValType>(m_variant) == std::get<AstFuncValType>(rhs.m_variant);
                }
            case Value::Type::HostFunction:
                {
                    return std::get<HostFuncValType>(m_variant) == std::get<HostFuncValType>(rhs.m_variant);
                }
            #endif
            case Value::Type::Object:
                {
                    return std::get<ObjectValType>(m_variant).get() == std::get<ObjectValType>(rhs.m_variant).get();
                }
            case Value::Type::Array:
                {
                    return std::get<ArrayValType>(m_variant).get() == std::get<ArrayValType>(rhs.m_variant).get();
                }
            case Value::Type::Type:
                {
                    return std::get<TypeValType>(m_variant) == std::get<TypeValType>(rhs.m_variant);
                }
            default:
                {
                    //assert(!"a Value::Type switch is missing in Value::isEqual()!");
                }
                break;
        }
        return false;
    }

    bool Value::isTrue() const
    {
        switch(m_type)
        {
            case Value::Type::Null:
                {
                    return false;
                }
                break;
            case Value::Type::Number:
                {
                    return std::get<NumberValType>(m_variant) != 0.f;
                }
                break;
            case Value::Type::String:
                {
                    return !std::get<StringValType>(m_variant).empty();
                }
                break;
            case Value::Type::Function:
            case Value::Type::Object:
            case Value::Type::HostFunction:
            case Value::Type::Array:
                {
                    return true;
                }
                break;
            case Value::Type::Type:
                {
                    return std::get<Value::Type>(m_variant) != Value::Type::Null;
                }
                break;
            default:
                {
                    assert(!"a Value::Type switch is missing in Value::isTrue()!");
                }
                break;
        }
        return false;
    }

    void Value::actualToStream(std::ostream& os, bool repr) const
    {
        switch(m_type)
        {
            case Value::Type::Null:
                {
                    os << "null";
                }
                break;
            case Value::Type::Number:
                {
                    os << getNumber();
                }
                break;
            case Value::Type::String:
                {
                    if(repr)
                    {
                        cppquote_string(os, getString());
                    }
                    else
                    {
                        os << getString();
                    }
                }
                break;
            case Value::Type::Function:
                {
                    auto p = getFunction();
                    os << "<astfunction @" << &p << ">";
                }
                break;
            case Value::Type::HostFunction:
                {
                    auto p = getHostFunction();
                    os << "<hostfunction @" << &p << ">";
                }
                break;
            case Value::Type::Object:
                {
                    auto obj = getObject();
                    os << "<object @" << &obj << ">";
                }
                break;
            case Value::Type::Array:
                {
                    dump_array(os, getArray(), repr);
                }
                break;
            case Value::Type::Type:
                {
                    os << "<type '"<< Value::getTypename(getTypeValue()) << "'>";
                }
                break;
            case Value::Type::MemberMethod:
                {
                    auto p = getMemberFunction();
                    os << "<member @" << &p << ">";
                }
                break;
            case Value::Type::MemberProperty:
                {
                    auto p = getPropertyFunction();
                    os << "<property @" << &p << ">";
                }
                break;
            default:
                {
                    os << "<value '" << Value::getTypename(m_type) << "'>";
                }
                break;
        }
    }

    std::string Value::toString() const
    {
        std::stringstream ss;
        toStream(ss);
        return ss.str();
    }

    std::string Value::toRepr() const
    {
        std::stringstream ss;
        reprToStream(ss);
        return ss.str();
    }

}
