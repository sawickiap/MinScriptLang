
#include <sstream>
#include "msl.h"

namespace MSL
{
    namespace
    {
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
                if(val.isArray() && (val.array() == arr))
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

        template<typename CharT>
        void dump_object(std::basic_ostream<CharT>& os, Object* obj, bool repr)
        {
            os << "Object{";
            for(auto iter = std::begin(obj->m_entrymap); iter != std::end(obj->m_entrymap); iter++)
            { 
                Util::reprString(os, iter->first);
                os << ": ";
                if(iter->second.isObject())
                {
                    if(iter->second.object() == obj)
                    {
                        os << "<recursion>";
                    }
                    else
                    {
                        iter->second.toStream(os, repr);
                    }
                }
                else
                {
                    iter->second.toStream(os, repr);
                }
                if (std::next(iter) != std::end(obj->m_entrymap))
                {
                    os << ", ";
                }
            }
            os << "}";
        }
    }

    void Array::markChildren()
    {
        for(auto& itm: m_arrayitems)
        {
            itm.mark();
        }
    }

    Array::Array()
    {
    }

    Array::~Array()
    {
    }

    void Value::markChildren()
    {
        switch(m_type)
        {
            case Type::Object:
                objectRef()->mark();
                break;
            case Type::Array:
                arrayRef()->mark();
                break;
            default:
                break;
        }
    }


    bool Value::isEqual(const Value& rhs) const
    {
        if(m_type != rhs.m_type)
        {
            return false;
        }
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
                    os << number();
                }
                break;
            case Value::Type::String:
                {
                    if(repr)
                    {
                        Util::reprString(os, string());
                    }
                    else
                    {
                        os << string();
                    }
                }
                break;
            case Value::Type::Function:
                {
                    auto p = scriptFunction();
                    os << "<scriptfunction @" << &p << ">";
                }
                break;
            case Value::Type::HostFunction:
                {
                    auto p = hostFunction();
                    os << "<nativefunction @" << &p << ">";
                }
                break;
            case Value::Type::Object:
                {
                    auto obj = object();
                    //os << "<object @" << &obj << ">";
                    dump_object(os, obj, repr);
                }
                break;
            case Value::Type::Array:
                {
                    dump_array(os, array(), repr);
                }
                break;
            case Value::Type::Type:
                {
                    os << "<type '"<< Value::getTypename(getTypeValue()) << "'>";
                }
                break;
            case Value::Type::MemberMethod:
                {
                    auto p = memberFunction();
                    os << "<memberfunction @" << &p << ">";
                }
                break;
            case Value::Type::MemberProperty:
                {
                    auto p = propertyFunction();
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
