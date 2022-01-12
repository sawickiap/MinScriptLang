
#include "msl.h"

namespace MSL
{
    namespace AST
    {

        Value ConstantExpression::execute(ExecutionContext& ctx) const
        {
            /* Nothing - just ignore its value. */

            #if 0
            const auto& th = ctx.getThis();
            if(!th.isEmpty())
            {
                return th.getObject();
            }
            #endif

            return {};
        }

    }

    
    namespace Builtins
    {
        Value func_typeof(Environment& env, const Location& place, std::vector<Value>&& args)
        {
            (void)env;
            if(args.size() == 0)
            {
                throw Error::ArgumentError(place, "typeof() requires exactly 1 argument");
            }
            return Value{ args[0].type() };
        }

        Value func_print(Environment& env, const Location& place, std::vector<Value>&& args)
        {
            (void)env;
            (void)place;
            for(const auto& val : args)
            {
                val.toStream(std::cout);
                std::cout << std::flush;
            }
            return {};
        }

        Value func_println(Environment& env, const Location& place, std::vector<Value>&& args)
        {
            (void)env;
            (void)place;
            for(const auto& val : args)
            {
                val.toStream(std::cout);
                std::cout << std::flush;
            }
            std::cout << std::endl;
            return {};
        }

        Value func_min(Environment& ctx, const Location& place, std::vector<Value>&& args)
        {
            size_t i;
            size_t argCount;
            double result;
            double argNum;
            (void)ctx;
            argCount = args.size();
            if(argCount == 0)
            {
                throw Error::ArgumentError(place, "Built-in function min requires at least 1 argument.");
            }
            result = 0.0;
            for(i = 0; i < argCount; ++i)
            {
                if(!args[i].isNumber())
                {
                    throw Error::ArgumentError(place, "Built-in function min requires number arguments.");
                }
                argNum = args[i].getNumber();
                if(i == 0 || argNum < result)
                {
                    result = argNum;
                }
            }
            return Value{ result };
        }

        Value func_max(Environment& ctx, const Location& place, std::vector<Value>&& args)
        {
            size_t i;
            size_t argCount;
            double argNum;
            double result;
            (void)ctx;
            argCount = args.size();
            if(argCount == 0)
            {
                throw Error::ArgumentError(place, "Built-in function min requires at least 1 argument.");
            }
            result = 0.0;
            for(i = 0; i < argCount; ++i)
            {
                if(!args[i].isNumber())
                {
                    throw Error::ArgumentError(place, "Built-in function min requires number arguments.");
                }
                argNum = args[i].getNumber();
                if(i == 0 || argNum > result)
                {
                    result = argNum;
                }
            }
            return Value{ result };
        }
    }

    Value EnvironmentPimpl::execute(std::string_view code)
    {
        AST::Script script{ Location{ 0, 1, 1 } };
        Tokenizer tokenizer{ code };
        Parser parser{ tokenizer };
        parser.ParseScript(script);
        try
        {
            AST::ExecutionContext executeContext{ *this, m_globalscope };
            return script.execute(executeContext);
        }
        catch(ReturnException& returnEx)
        {
            return std::move(returnEx.thrownvalue);
        }
        return {};
    }

    std::string_view EnvironmentPimpl::getTypeName(Value::Type type) const
    {
        //return VALUE_TYPE_NAMES[(size_t)type];
        // TODO support custom types
        return Value::getTypeName(type);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Environment

    Environment::Environment() : m_implenv{ new EnvironmentPimpl{ *this, m_globalscope } }
    {
        global("min") = Value{Builtins::func_min};
        global("typeOf") = Value{Builtins::func_typeof};
        global("max") = Value{Builtins::func_max};
        global("print") = Value{Builtins::func_print};
        global("println") = Value{Builtins::func_println};
    }

    Environment::~Environment()
    {
        delete m_implenv;
    }

    Value Environment::execute(std::string_view code)
    {
        return m_implenv->execute(code);
    }

    std::string_view Environment::getTypeName(Value::Type type) const
    {
        return m_implenv->getTypeName(type);
    }

    void Environment::Print(std::string_view s)
    {
        m_implenv->Print(s);
    }
}


