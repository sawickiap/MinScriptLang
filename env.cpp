
#include "msl.h"

namespace MSL
{

    Value EnvironmentPimpl::execute(const std::string_view& code)
    {
        AST::Script script{ PlaceInCode{ 0, 1, 1 } };
        {
            Tokenizer tokenizer{ code };
            Parser parser{ tokenizer };
            parser.ParseScript(script);
        }

        try
        {
            AST::ExecutionContext executeContext{ *this, m_globalscope };
            script.execute(executeContext);
        }
        catch(ReturnException& returnEx)
        {
            return std::move(returnEx.thrownvalue);
        }
        return {};
    }

    std::string_view EnvironmentPimpl::getTypeName(Value::Type type) const
    {
        return VALUE_TYPE_NAMES[(size_t)type];
        // TODO support custom types
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Environment

    Environment::Environment() : m_implenv{ new EnvironmentPimpl{ *this, m_globalscope } }
    {
        global("min") = Value{Builtins::func_min};
        global("typeOf") = Value{Builtins::func_typeof};
        global("max") = Value{Builtins::func_max};
        global("print") = Value{Builtins::func_print};
    }

    Environment::~Environment()
    {
        delete m_implenv;
    }

    Value Environment::execute(const std::string_view& code)
    {
        return m_implenv->execute(code);
    }

    std::string_view Environment::getTypeName(Value::Type type) const
    {
        return m_implenv->getTypeName(type);
    }

    void Environment::Print(const std::string_view& s)
    {
        m_implenv->Print(s);
    }
}


