
#include "priv.h"



namespace MSL
{
    namespace Builtins
    {
        static void fmtfunc_checkargs(std::string_view n, const Location& p, const Value::List& args)
        {
            if(args.size() == 0)
            {
                throw Error::ArgumentError(p, Util::joinArgs(n, "() needs at least 1 argument"));
            }
            if(!args[0].isString())
            {
                throw Error::TypeError(p, Util::joinArgs(n, "() expects a string as first argument"));
            }
        }


        Value func_typeof(Environment& env, const Location& place, Value::List&& args)
        {
            (void)env;
            if(args.size() == 0)
            {
                throw Error::ArgumentError(place, "typeof() requires exactly 1 argument");
            }
            return Value{ args[0].type() };
        }

        Value func_min(Environment& ctx, const Location& place, Value::List&& args)
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
                argNum = args[i].number();
                if(i == 0 || argNum < result)
                {
                    result = argNum;
                }
            }
            return Value{ result };
        }

        Value func_max(Environment& ctx, const Location& place, Value::List&& args)
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
                argNum = args[i].number();
                if(i == 0 || argNum > result)
                {
                    result = argNum;
                }
            }
            return Value{ result };
        }

        Value func_print(Environment& env, const Location& place, Value::List&& args)
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

        Value func_println(Environment& env, const Location& place, Value::List&& args)
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

        Value func_sprintf(Environment& env, const Location& place, Value::List&& args)
        {
            std::stringstream ss;
            std::string fmtstr;
            fmtfunc_checkargs("sprintf", place, args);
            fmtstr = args[0].toString();
            auto restargs = Value::List(args.begin() + 1, args.end());
            Util::StringFormatter(env, place, fmtstr, restargs).run(ss, false);
            return Value{ss.str()};
        }

        Value func_printf(Environment& env, const Location& place, Value::List&& args)
        {
            std::string fmtstr;
            fmtfunc_checkargs("printf", place, args);
            fmtstr = args[0].toString();
            auto restargs = Value::List(args.begin() + 1, args.end());
            Util::StringFormatter(env, place, fmtstr, restargs).run(std::cout, true);
            return {};
        }
    }
}
