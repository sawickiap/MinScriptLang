
#include <sstream>
#include "msl.h"

#undef assert
#define assert(...)

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

    /*
    void Array::markChildren()
    {
        for(auto& itm: m_arrayitems)
        {
            itm.mark();
        }
    }
    */

    void Number::postInitCheck(bool wascopy)
    {
        
    }

    Array::Array()
    {
    }

    Array::~Array()
    {
    }

    Value::Value()
    {
    }

    Value::Value(Value::NumberValType number) : m_type(Value::Type::Number), m_variant(number)
    {
    }

    Value::Value(Value::StringValType&& str) : m_type(Value::Type::String), m_variant(std::move(str))
    {
    }

    Value::Value(Value::AstFuncValType func) : m_type{ Value::Type::Function }, m_variant{ func }
    {
    }

    Value::Value(HostFunction func) : m_type{ Value::Type::HostFunction }, m_variant{ func }
    {
        assert(func);
    }

    Value::Value(MemberMethodFunction func): m_type{Value::Type::MemberMethod}, m_variant{func}
    {
    }

    Value::Value(MemberPropertyFunction func): m_type{Value::Type::MemberProperty}, m_variant{func}
    {
    }        

    Value::Value(Value::ObjectValType&& obj) : m_type{ Value::Type::Object }, m_variant(obj)
    {
    }

    Value::Value(Value::ArrayValType&& arr) : m_type{ Value::Type::Array }, m_variant(arr)
    {
    }

    Value::Value(Value::Type typeVal) : m_type{ Value::Type::Type }, m_variant(typeVal)
    {
    }

    Value::Type Value::type() const
    {
        return m_type;
    }

    const Location& Value::location() const
    {
        return m_location;
    }

    Location& Value::location()
    {
        return m_location;
    }

    const Value::NumberValType& Value::number() const
    {
        assert(m_type == Value::Type::Number);
        return std::get<Value::NumberValType>(m_variant);
    }

    Value::StringValType& Value::string()
    {
        assert(m_type == Value::Type::String);
        return std::get<Value::StringValType>(m_variant);
    }

    const Value::StringValType& Value::string() const
    {
        assert(m_type == Value::Type::String);
        return std::get<Value::StringValType>(m_variant);
    }

    Value::AstFuncValType Value::scriptFunction()
    {
        assert(m_type == Value::Type::Function && std::get<Value::AstFuncValType>(m_variant));
        return std::get<Value::AstFuncValType>(m_variant);
    }

    Value::AstFuncValType Value::scriptFunction() const
    {
        assert(m_type == Value::Type::Function && std::get<Value::AstFuncValType>(m_variant));
        return std::get<Value::AstFuncValType>(m_variant);
    }

    Value::HostFuncValType Value::hostFunction() const
    {
        assert(m_type == Value::Type::HostFunction);
        return std::get<Value::HostFuncValType>(m_variant);
    }

    Value::MemberFuncValType Value::memberFunction() const
    {
        assert(m_type == Value::Type::MemberMethod);
        return std::get<Value::MemberFuncValType>(m_variant);
    }

    Value::MemberPropValType Value::propertyFunction() const
    {
        assert(m_type == Value::Type::MemberProperty);
        return std::get<Value::MemberPropValType>(m_variant);
    }

    Object* Value::object() const
    {
        assert(m_type == Value::Type::Object && std::get<Value::ObjectValType>(m_variant));
        return std::get<Value::ObjectValType>(m_variant).get();
    }

    Value::ObjectValType Value::objectRef() const
    {
        assert(m_type == Value::Type::Object && std::get<Value::ObjectValType>(m_variant));
        return std::get<Value::ObjectValType>(m_variant);
    }

    Array* Value::array() const
    {
        assert(m_type == Value::Type::Array && std::get<Value::ArrayValType>(m_variant));
        return std::get<Value::ArrayValType>(m_variant).get();
    }

    Value::ArrayValType Value::arrayRef() const
    {
        assert(m_type == Value::Type::Array && std::get<Value::ArrayValType>(m_variant));
        return std::get<Value::ArrayValType>(m_variant);
    }

    Value::Type Value::getTypeValue() const
    {
        assert(m_type == Value::Type::Type);
        return std::get<Value::TypeValType>(m_variant);
    }

    void Value::setNumberValue(Value::NumberValType number)
    {
        assert(m_type == Value::Type::Number);
        m_variant = number;
    }

    bool Value::isNull() const
    {
        return (m_type == Value::Type::Null);
    }

    bool Value::isObject() const
    {
        return (m_type == Value::Type::Object);
    }

    bool Value::isNumber() const
    {
        return (m_type == Value::Type::Number);
    }

    bool Value::isString() const
    {
        return (m_type == Value::Type::String);
    }

    bool Value::isArray() const
    {
        return (m_type == Value::Type::Array);
    }

    /*
    void Value::markChildren()
    {
        switch(m_type)
        {
            case Value::Type::Object:
                objectRef()->mark();
                break;
            case Value::Type::Array:
                arrayRef()->mark();
                break;
            default:
                break;
        }
    }
    */

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
                    return std::get<Value::NumberValType>(m_variant) == std::get<Value::NumberValType>(rhs.m_variant);
                }
            case Value::Type::String:
                {
                    return std::get<Value::StringValType>(m_variant) == std::get<Value::StringValType>(rhs.m_variant);
                }
            #if 0
            case Value::Type::Function:
                {
                    return std::get<Value::AstFuncValType>(m_variant) == std::get<Value::AstFuncValType>(rhs.m_variant);
                }
            case Value::Type::HostFunction:
                {
                    return std::get<Value::HostFuncValType>(m_variant) == std::get<Value::HostFuncValType>(rhs.m_variant);
                }
            #endif
            case Value::Type::Object:
                {
                    return std::get<Value::ObjectValType>(m_variant).get() == std::get<Value::ObjectValType>(rhs.m_variant).get();
                }
            case Value::Type::Array:
                {
                    return std::get<Value::ArrayValType>(m_variant).get() == std::get<Value::ArrayValType>(rhs.m_variant).get();
                }
            case Value::Type::Type:
                {
                    return std::get<Value::TypeValType>(m_variant) == std::get<Value::TypeValType>(rhs.m_variant);
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
                    auto n = std::get<Value::NumberValType>(m_variant);
                    if(n.isInteger())
                    {
                        return n.toInteger() != 0;
                    }
                    return n.toFloat() != 0.f;
                }
                break;
            case Value::Type::String:
                {
                    return !std::get<Value::StringValType>(m_variant).empty();
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
                    if(number().isInteger())
                    {
                        os << number().toInteger();
                    }
                    else
                    {
                        os << number().toFloat();
                    }
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
