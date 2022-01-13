
#include "priv.h"

// Convenience macros for loading function arguments
// They require to have following available: env, place, args.
#define MINSL_LOAD_ARG_BEGIN(functionNameStr)           \
    const char* minsl_functionName = (functionNameStr); \
    size_t minsl_argIndex = 0;                          \
    size_t minsl_argCount = (args).size();              \
    Value::Type minsl_argType;
#define MINSL_LOAD_ARG_NUMBER(dstVarName)                                                                          \
    MINSL_EXECUTION_CHECK(minsl_argIndex < minsl_argCount, place,                                                  \
                          Util::Format("Function %s received too few arguments. Number expected as argument %zu.",       \
                                 minsl_functionName, minsl_argIndex));                                             \
    minsl_argType = args[minsl_argIndex].type();                                                                \
    MINSL_EXECUTION_CHECK(minsl_argType == Value::Type::Number, place,                                               \
                          Util::Format("Function %s received incorrect argument %zu. Expected: Number, actual: %.*s.",   \
                                 minsl_functionName, minsl_argIndex, (int)env.getTypename(minsl_argType).length(), \
                                 env.getTypename(minsl_argType).data()));                                          \
    double dstVarName = args[minsl_argIndex++].getNumber();
#define MINSL_LOAD_ARG_STRING(dstVarName)                                                                          \
    MINSL_EXECUTION_CHECK(minsl_argIndex < minsl_argCount, place,                                                  \
                          Util::Format("Function %s received too few arguments. String expected as argument %zu.",       \
                                 minsl_functionName, minsl_argIndex));                                             \
    minsl_argType = args[minsl_argIndex].type();                                                                \
    MINSL_EXECUTION_CHECK(minsl_argType == Value::Type::String, place,                                               \
                          Util::Format("Function %s received incorrect argument %zu. Expected: String, actual: %.*s.",   \
                                 minsl_functionName, minsl_argIndex, (int)env.getTypename(minsl_argType).length(), \
                                 env.getTypename(minsl_argType).data()));                                          \
    std::string dstVarName = std::move(args[minsl_argIndex++].getString());
#define MINSL_LOAD_ARG_END()                 \
    MINSL_EXECUTION_CHECK(                   \
    minsl_argIndex == minsl_argCount, place, \
    Util::Format("Function %s requires %zu arguments, %zu provided.", minsl_functionName, minsl_argIndex, minsl_argCount));

#define MINSL_LOAD_ARGS_0(functionNameStr) \
    MINSL_LOAD_ARG_BEGIN(functionNameStr); \
    MINSL_LOAD_ARG_END();
#define MINSL_LOAD_ARGS_1_NUMBER(functionNameStr, dstVarName) \
    MINSL_LOAD_ARG_BEGIN(functionNameStr);                    \
    MINSL_LOAD_ARG_NUMBER(dstVarName);                        \
    MINSL_LOAD_ARG_END();
#define MINSL_LOAD_ARGS_1_STRING(functionNameStr, dstVarName) \
    MINSL_LOAD_ARG_BEGIN(functionNameStr);                    \
    MINSL_LOAD_ARG_STRING(dstVarName);                        \
    MINSL_LOAD_ARG_END();
#define MINSL_LOAD_ARGS_2_NUMBERS(functionNameStr, dstVarName1, dstVarName2) \
    MINSL_LOAD_ARG_BEGIN(functionNameStr);                                   \
    MINSL_LOAD_ARG_NUMBER(dstVarName1);                                      \
    MINSL_LOAD_ARG_NUMBER(dstVarName2);                                      \
    MINSL_LOAD_ARG_END();

namespace MSL
{
    namespace Builtins
    {
        Value ctor_null(AST::ExecutionContext& ctx, const Location& place, std::vector<Value>&& args)
        {
            (void)ctx;
            (void)place;
            (void)args;
            return {};
        }

        Value ctor_number(AST::ExecutionContext& ctx, const Location& place, std::vector<Value>&& args)
        {
            Environment& env = ctx.m_env.getOwner();
            MINSL_LOAD_ARGS_1_NUMBER("Number", val);
            return Value{ val };
        }

        Value ctor_string(AST::ExecutionContext& ctx, const Location& place, std::vector<Value>&& args)
        {
            if(args.empty())
            {
                return Value{ std::string{} };
            }
            Environment& env = ctx.m_env.getOwner();
            MINSL_LOAD_ARGS_1_STRING("String", str);
            return Value{ std::move(str) };
        }

        Value ctor_object(AST::ExecutionContext& ctx, const Location& place, std::vector<Value>&& args)
        {
            (void)ctx;
            if(args.empty())
            {
                return Value{ std::make_shared<Object>() };
            }
            if((args.size() == 0) || !args[0].isObject())
            {
                throw Error::ArgumentError(place, "Object can be constructed only from no arguments or from another object value.");
            }
            return Value{ Util::CopyObject(*args[0].getObject()) };
        }

        Value ctor_array(AST::ExecutionContext& ctx, const Location& place, std::vector<Value>&& args)
        {
            (void)ctx;
            if(args.empty())
            {
                return Value{ std::make_shared<Array>() };
            }
            if((args.size() == 0) || !args[0].isArray())
            {
                throw Error::ArgumentError(place, "Array can be constructed only from no arguments or from another array value.");
            }
            return Value{ Util::CopyArray(*args[0].getArray()) };
        }

        Value ctor_function(AST::ExecutionContext& ctx, const Location& place, std::vector<Value>&& args)
        {
            (void)ctx;
            if((args.size() == 0) || ((args[0].type() != Value::Type::Function) && (args[0].type() != Value::Type::MemberMethod)))
            {
                throw Error::ArgumentError(place, "Function can be constructed only from another function value.");
            }
            return Value{ args[0] };
        }

        Value ctor_type(AST::ExecutionContext& ctx, const Location& place, std::vector<Value>&& args)
        {
            (void)ctx;
            if((args.size() == 0) || (args[0].type() != Value::Type::Type))
            {
                
                throw Error::ArgumentError(place, "Type can be constructed only from another type value.");
            }
            return Value{ args[0] };
        }

        Value protofn_object_count(AST::ExecutionContext& ctx, const Location& place, Value&& objVal)
        {
            (void)ctx;
            Object* obj;
            if(!objVal.isObject() || ((obj = objVal.getObject()) == nullptr))
            {
                throw Error::TypeError(place, "Object() requires an object");
            }
            return Value{ (double)obj->size() };
        }

        Value protofn_array_length(AST::ExecutionContext& ctx, const Location& place, Value&& objVal)
        {
            (void)ctx;
            Array* arr;
            if(!objVal.isArray() || ((arr = objVal.getArray()) == nullptr))
            {
                throw Error::TypeError(place, "Array.length called on something not an Array");
            }
            return Value{ (double)objVal.getArray()->m_items.size() };
        }


        Value memberfn_array_add(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, std::vector<Value>&& args)
        {
            size_t i;
            Array* arr;
            (void)ctx;
            (void)th;
            arr = th.getArray();
            if(!arr)
            {
                throw Error::TypeError(place, "Array.add() called on something not an Array");
            }
            if(args.size() == 0)
            {
                throw Error::ArgumentError(place, "Array.add() requires at least 1 argument");
            }
            for(i=0; i<args.size(); i++)
            {
                arr->m_items.push_back(std::move(args[i]));
            }
            return {};
        }

        Value memberfn_array_insert(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, std::vector<Value>&& args)
        {
            Array* arr;
            (void)ctx;
            arr = th.getArray();
            if(!arr)
            {
                throw Error::ArgumentError(place, "Array.insert() called on something not an Array");
            }
            if(args.size() != 2)
            {
                throw Error::ArgumentError(place, "Array.insert() requires at least 2 arguments");
            }
            size_t index;
            index= 0;
            if(!args[0].isNumber() || !Util::NumberToIndex(index, args[0].getNumber()))
            {
                throw Error::ExecutionError(place, "cannot insert out-of-bounds");
            }
            arr->m_items.insert(arr->m_items.begin() + index, std::move(args[1]));
            return {};
        }

        Value memberfn_array_remove(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, std::vector<Value>&& args)
        {
            Array* arr;
            (void)ctx;
            arr = th.getArray();
            if(!arr)
            {
                throw Error::TypeError(place, "expected array object");
            }
            if(args.size() == 0)
            {
                throw Error::TypeError(place, "too few arguments");
            }
            size_t index = 0;
            if(!args[0].isNumber() || !Util::NumberToIndex(index, args[0].getNumber()))
            {
                throw Error::ExecutionError(place, "cannot remove out-of-bounds");
            }
            arr->m_items.erase(arr->m_items.begin() + index);
            return {};
        }

        Value protofn_string_length(AST::ExecutionContext& ctx, const Location& place, Value&& objVal)
        {
            (void)ctx;
            if(!objVal.isString())
            {
                throw Error::TypeError(place, "expected string object");
            }
            return Value{ (double)objVal.getString().length() };
        }

        Value protofn_string_chars(AST::ExecutionContext& ctx, const Location& place, Value&& objVal)
        {
            
            (void)ctx;
            if(!objVal.isString())
            {
                throw Error::TypeError(place, "expected string object");
            }
            auto res = std::make_shared<Array>();
            for(auto ch: objVal.getString())
            {
                res->m_items.push_back(Value{double(ch)});
            }
            return Value{ std::move(res) };
        }

        Value memberfn_string_resize(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, std::vector<Value>&& args)
        {
            // TODO
            (void)ctx;
            (void)place;
            (void)th;
            (void)args;
            return {};
        }

        Value memberfn_string_startswith(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, std::vector<Value>&& args)
        {
            double res;
            (void)ctx;
            if(args.size() == 0)
            {
                throw Error::ArgumentError(place, "String.startsWith() needs exactly 1 argument");
            }
            auto findme = args[0];
            res = (th.getString().rfind(findme.getString(), 0) == 0);
            return Value{res};
        }
    }
}

