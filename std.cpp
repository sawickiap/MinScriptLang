
#include "msl.h"

// Convenience macros for loading function arguments
// They require to have following available: env, place, args.
#define MINSL_LOAD_ARG_BEGIN(functionNameStr)           \
    const char* minsl_functionName = (functionNameStr); \
    size_t minsl_argIndex = 0;                          \
    size_t minsl_argCount = (args).size();              \
    Value::Type minsl_argType;
#define MINSL_LOAD_ARG_NUMBER(dstVarName)                                                                          \
    MINSL_EXECUTION_CHECK(minsl_argIndex < minsl_argCount, place,                                                  \
                          Format("Function %s received too few arguments. Number expected as argument %zu.",       \
                                 minsl_functionName, minsl_argIndex));                                             \
    minsl_argType = args[minsl_argIndex].type();                                                                \
    MINSL_EXECUTION_CHECK(minsl_argType == Value::Type::Number, place,                                               \
                          Format("Function %s received incorrect argument %zu. Expected: Number, actual: %.*s.",   \
                                 minsl_functionName, minsl_argIndex, (int)env.getTypeName(minsl_argType).length(), \
                                 env.getTypeName(minsl_argType).data()));                                          \
    double dstVarName = args[minsl_argIndex++].getNumber();
#define MINSL_LOAD_ARG_STRING(dstVarName)                                                                          \
    MINSL_EXECUTION_CHECK(minsl_argIndex < minsl_argCount, place,                                                  \
                          Format("Function %s received too few arguments. String expected as argument %zu.",       \
                                 minsl_functionName, minsl_argIndex));                                             \
    minsl_argType = args[minsl_argIndex].type();                                                                \
    MINSL_EXECUTION_CHECK(minsl_argType == Value::Type::String, place,                                               \
                          Format("Function %s received incorrect argument %zu. Expected: String, actual: %.*s.",   \
                                 minsl_functionName, minsl_argIndex, (int)env.getTypeName(minsl_argType).length(), \
                                 env.getTypeName(minsl_argType).data()));                                          \
    std::string dstVarName = std::move(args[minsl_argIndex++].getString());
#define MINSL_LOAD_ARG_END()                 \
    MINSL_EXECUTION_CHECK(                   \
    minsl_argIndex == minsl_argCount, place, \
    Format("Function %s requires %zu arguments, %zu provided.", minsl_functionName, minsl_argIndex, minsl_argCount));

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
    static std::shared_ptr<Array> CopyArray(const Array& src)
    {
        auto dst = std::make_shared<Array>();
        const size_t count = src.m_items.size();
        dst->m_items.resize(count);
        for(size_t i = 0; i < count; ++i)
            dst->m_items[i] = Value{ src.m_items[i] };
        return dst;
    }

    namespace Builtins
    {
        Value ctor_null(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
        {
            (void)ctx;
            MINSL_EXECUTION_CHECK((args.empty() || ((args.size() == 1) && (args[0].isNull()))), place,
                                  "Null can be constructed only from no arguments or from another null value.");
            return {};
        }

        Value ctor_number(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
        {
            Environment& env = ctx.m_env.getOwner();
            MINSL_LOAD_ARGS_1_NUMBER("Number", val);
            return Value{ val };
        }

        Value ctor_string(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
        {
            if(args.empty())
            {
                return Value{ std::string{} };
            }
            Environment& env = ctx.m_env.getOwner();
            MINSL_LOAD_ARGS_1_STRING("String", str);
            return Value{ std::move(str) };
        }

        Value ctor_object(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
        {
            (void)ctx;
            if(args.empty())
            {
                return Value{ std::make_shared<Object>() };
            }
            MINSL_EXECUTION_CHECK(((args.size() == 1) && (args[0].isObject())), place,
                                  "Object can be constructed only from no arguments or from another object value.");
            return Value{ CopyObject(*args[0].getObject()) };
        }

        Value ctor_array(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
        {
            (void)ctx;
            if(args.empty())
            {
                return Value{ std::make_shared<Array>() };
            }
            MINSL_EXECUTION_CHECK(((args.size() == 1) && (args[0].isArray())), place,
                                  "Array can be constructed only from no arguments or from another array value.");
            return Value{ CopyArray(*args[0].getArray()) };
        }

        Value ctor_function(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
        {
            (void)ctx;
            MINSL_EXECUTION_CHECK(
                ((args.size() == 1) && ((args[0].type() == Value::Type::Function) || (args[0].type() == Value::Type::SystemFunction))),
                                  place, "Function can be constructed only from another function value.");
            return Value{ args[0] };
        }

        Value ctor_type(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
        {
            (void)ctx;
            MINSL_EXECUTION_CHECK(
                ((args.size() == 1) && (args[0].type() == Value::Type::Type)), place,
                                  "Type can be constructed only from another type value.");
            return Value{ args[0] };
        }

        Value func_typeof(Environment& env, const PlaceInCode& place, std::vector<Value>&& args)
        {
            (void)env;
            MINSL_EXECUTION_CHECK((args.size() == 1), place, ERROR_MESSAGE_EXPECTED_1_ARGUMENT);
            return Value{ args[0].type() };
        }

        Value func_print(Environment& env, const PlaceInCode& place, std::vector<Value>&& args)
        {
            std::string s;
            (void)place;
            for(const auto& val : args)
            {
                switch(val.type())
                {
                    case Value::Type::Null:
                        env.Print("null");
                        break;
                    case Value::Type::Number:
                        s = Format("%g", val.getNumber());
                        env.Print(s);
                        break;
                    case Value::Type::String:
                        if(!val.getString().empty())
                        {
                            env.Print(val.getString());
                        }
                        break;
                    case Value::Type::Function:
                    case Value::Type::SystemFunction:
                    case Value::Type::HostFunction:
                        env.Print("function");
                        break;
                    case Value::Type::Object:
                        env.Print("object");
                        break;
                    case Value::Type::Array:
                        env.Print("array");
                        break;
                    case Value::Type::Type:
                    {
                        const size_t typeIndex = (size_t)val.getTypeValue();
                        const std::string_view& typeName = VALUE_TYPE_NAMES[typeIndex];
                        s = Format("%.*s", (int)typeName.length(), typeName.data());
                        env.Print(s);
                    }
                    break;
                    default:
                        assert(0);
                }
            }
            return {};
        }
        Value func_min(Environment& ctx, const PlaceInCode& place, std::vector<Value>&& args)
        {
            size_t i;
            size_t argCount;
            double result;
            double argNum;
            (void)ctx;
            argCount = args.size();
            MINSL_EXECUTION_CHECK((argCount > 0), place, "Built-in function min requires at least 1 argument.");
            result = 0.0;
            for(i = 0; i < argCount; ++i)
            {
                MINSL_EXECUTION_CHECK(args[i].isNumber(), place, "Built-in function min requires number arguments.");
                argNum = args[i].getNumber();
                if(i == 0 || argNum < result)
                {
                    result = argNum;
                }
            }
            return Value{ result };
        }
        Value func_max(Environment& ctx, const PlaceInCode& place, std::vector<Value>&& args)
        {
            size_t i;
            size_t argCount;
            double argNum;
            double result;
            (void)ctx;
            argCount = args.size();
            MINSL_EXECUTION_CHECK((argCount > 0), place, "Built-in function min requires at least 1 argument.");
            result = 0.0;
            for(i = 0; i < argCount; ++i)
            {
                MINSL_EXECUTION_CHECK(args[i].isNumber(), place, "Built-in function min requires number arguments.");
                argNum = args[i].getNumber();
                if(i == 0 || argNum > result)
                {
                    result = argNum;
                }
            }
            return Value{ result };
        }

        Value memberfn_object_count(AST::ExecutionContext& ctx, const PlaceInCode& place, Value&& objVal)
        {
            (void)ctx;
            MINSL_EXECUTION_CHECK(((objVal.isObject()) && (objVal.getObject())), place, ERROR_MESSAGE_EXPECTED_OBJECT);
            return Value{ (double)objVal.getObject()->size() };
        }

        Value memberfn_array_count(AST::ExecutionContext& ctx, const PlaceInCode& place, Value&& objVal)
        {
            (void)ctx;
            MINSL_EXECUTION_CHECK(((objVal.isArray()) && objVal.getArray()), place, ERROR_MESSAGE_EXPECTED_ARRAY);
            return Value{ (double)objVal.getArray()->m_items.size() };
        }

        Value memberfn_string_count(AST::ExecutionContext& ctx, const PlaceInCode& place, Value&& objVal)
        {
            (void)ctx;
            MINSL_EXECUTION_CHECK((objVal.isString()), place, ERROR_MESSAGE_EXPECTED_STRING);
            return Value{ (double)objVal.getString().length() };
        }

        Value memberfn_string_resize(AST::ExecutionContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args)
        {
            // TODO
            (void)ctx;
            (void)place;
            (void)th;
            (void)args;
            return {};
        }

        Value memberfn_array_add(AST::ExecutionContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args)
        {
            Array* arr;
            (void)ctx;
            (void)th;
            arr = th.getArray();
            MINSL_EXECUTION_CHECK(arr, place, ERROR_MESSAGE_EXPECTED_ARRAY);
            MINSL_EXECUTION_CHECK(args.size() == 1, place, ERROR_MESSAGE_EXPECTED_1_ARGUMENT);
            arr->m_items.push_back(std::move(args[0]));
            return {};
        }

        Value memberfn_array_insert(AST::ExecutionContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args)
        {
            Array* arr;
            (void)ctx;
            arr = th.getArray();
            MINSL_EXECUTION_CHECK(arr, place, ERROR_MESSAGE_EXPECTED_ARRAY);
            MINSL_EXECUTION_CHECK(args.size() == 2, place, ERROR_MESSAGE_EXPECTED_2_ARGUMENTS);
            size_t index = 0;
            MINSL_EXECUTION_CHECK(args[0].isNumber() && NumberToIndex(index, args[0].getNumber()),
                                  place, "cannot insert out-of-bounds");
            arr->m_items.insert(arr->m_items.begin() + index, std::move(args[1]));
            return {};
        }

        Value memberfn_array_remove(AST::ExecutionContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args)
        {
            Array* arr;
            (void)ctx;
            arr = th.getArray();
            MINSL_EXECUTION_CHECK(arr, place, ERROR_MESSAGE_EXPECTED_ARRAY);
            MINSL_EXECUTION_CHECK(args.size() == 1, place, ERROR_MESSAGE_EXPECTED_1_ARGUMENT);
            size_t index = 0;
            MINSL_EXECUTION_CHECK(args[0].isNumber() && NumberToIndex(index, args[0].getNumber()),
                                  place, "cannot remove out-of-bounds");
            arr->m_items.erase(arr->m_items.begin() + index);
            return {};
        }
    }
}

