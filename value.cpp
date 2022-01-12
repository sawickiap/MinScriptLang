
#include "msl.h"

namespace MSL
{
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
            case Value::Type::Function:
                {
                    return std::get<AstFuncValType>(m_variant) == std::get<AstFuncValType>(rhs.m_variant);
                }
            case Value::Type::HostFunction:
                {
                    return std::get<HostFuncValType>(m_variant) == std::get<HostFuncValType>(rhs.m_variant);
                }
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
                    assert(!"a Value::Type switch is missing in Value::isEqual()!");
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
}
