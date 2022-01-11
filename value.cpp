
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
                return true;
            case Value::Type::Number:
                return std::get<double>(m_variant) == std::get<double>(rhs.m_variant);
            case Value::Type::String:
                return std::get<std::string>(m_variant) == std::get<std::string>(rhs.m_variant);
            case Value::Type::Function:
                return std::get<const AST::FunctionDefinition*>(m_variant)
                       == std::get<const AST::FunctionDefinition*>(rhs.m_variant);
            case Value::Type::SystemFunction:
                return std::get<SystemFunction>(m_variant) == std::get<SystemFunction>(rhs.m_variant);
            case Value::Type::HostFunction:
                return std::get<HostFunction*>(m_variant) == std::get<HostFunction*>(rhs.m_variant);
            case Value::Type::Object:
                return std::get<std::shared_ptr<Object>>(m_variant).get()
                       == std::get<std::shared_ptr<Object>>(rhs.m_variant).get();
            case Value::Type::Array:
                return std::get<std::shared_ptr<Array>>(m_variant).get()
                       == std::get<std::shared_ptr<Array>>(rhs.m_variant).get();
            case Value::Type::Type:
                return std::get<Value::Type>(m_variant) == std::get<Value::Type>(rhs.m_variant);
            default:
                assert(0);
                return false;
        }
    }

    bool Value::isTrue() const
    {
        switch(m_type)
        {
            case Value::Type::Null:
                return false;
            case Value::Type::Number:
                return std::get<double>(m_variant) != 0.f;
            case Value::Type::String:
                return !std::get<std::string>(m_variant).empty();
            case Value::Type::Function:
                return true;
            case Value::Type::SystemFunction:
                return true;
            case Value::Type::HostFunction:
                return true;
            case Value::Type::Object:
                return true;
            case Value::Type::Array:
                return true;
            case Value::Type::Type:
                return std::get<Value::Type>(m_variant) != Value::Type::Null;
            default:
                assert(0);
                return false;
        }
    }

}

