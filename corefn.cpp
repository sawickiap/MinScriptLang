
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

        Value func_eval(Environment& env, const Location& loc, Value::List&& args)
        {
            Value strv;
            Util::ArgumentCheck ac(loc, args, "eval");
            strv = ac.checkArgument(0, {Value::Type::String});
            return env.execute(strv.string());
        }

        Value func_load(Environment& env, const Location& loc, Value::List&& args)
        {
            Value filev;
            Util::ArgumentCheck ac(loc, args, "load");
            filev = ac.checkArgument(0, {Value::Type::String});
            auto d = Util::readFile(filev.string());
            if(!d)
            {
                throw Error::IOError(loc, Util::joinArgs("failed to read file '", filev.string(), "' for 'load'"));
            }
            return env.execute(d.value(), filev.string());
        }


        Value func_typeof(Environment& env, const Location& loc, Value::List&& args)
        {
            (void)env;
            if(args.size() == 0)
            {
                throw Error::ArgumentError(loc, "typeof() requires exactly 1 argument");
            }
            return Value{ args[0].type() };
        }

        Value func_min(Environment& ctx, const Location& loc, Value::List&& args)
        {
            size_t i;
            size_t argCount;
            double result;
            double argNum;
            (void)ctx;
            argCount = args.size();
            if(argCount == 0)
            {
                throw Error::ArgumentError(loc, "Built-in function min requires at least 1 argument.");
            }
            result = 0.0;
            for(i = 0; i < argCount; ++i)
            {
                if(!args[i].isNumber())
                {
                    throw Error::ArgumentError(loc, "Built-in function min requires number arguments.");
                }
                argNum = args[i].number();
                if(i == 0 || argNum < result)
                {
                    result = argNum;
                }
            }
            return Value{ result };
        }

        Value func_max(Environment& ctx, const Location& loc, Value::List&& args)
        {
            size_t i;
            size_t argCount;
            double argNum;
            double result;
            (void)ctx;
            argCount = args.size();
            if(argCount == 0)
            {
                throw Error::ArgumentError(loc, "Built-in function min requires at least 1 argument.");
            }
            result = 0.0;
            for(i = 0; i < argCount; ++i)
            {
                if(!args[i].isNumber())
                {
                    throw Error::ArgumentError(loc, "Built-in function min requires number arguments.");
                }
                argNum = args[i].number();
                if(i == 0 || argNum > result)
                {
                    result = argNum;
                }
            }
            return Value{ result };
        }

        Value func_print(Environment& env, const Location& loc, Value::List&& args)
        {
            (void)env;
            (void)loc;
            for(const auto& val : args)
            {
                val.toStream(std::cout);
                std::cout << std::flush;
            }
            return {};
        }

        Value func_println(Environment& env, const Location& loc, Value::List&& args)
        {
            (void)env;
            (void)loc;
            for(const auto& val : args)
            {
                val.toStream(std::cout);
                std::cout << std::flush;
            }
            std::cout << std::endl;
            return {};
        }

        Value func_sprintf(Environment& env, const Location& loc, Value::List&& args)
        {
            std::stringstream ss;
            std::string fmtstr;
            fmtfunc_checkargs("sprintf", loc, args);
            fmtstr = args[0].toString();
            auto restargs = Value::List(args.begin() + 1, args.end());
            Util::StringFormatter(env, loc, fmtstr, restargs).run(ss, false);
            return Value{ss.str()};
        }

        Value func_printf(Environment& env, const Location& loc, Value::List&& args)
        {
            std::string fmtstr;
            fmtfunc_checkargs("printf", loc, args);
            fmtstr = args[0].toString();
            auto restargs = Value::List(args.begin() + 1, args.end());
            Util::StringFormatter(env, loc, fmtstr, restargs).run(std::cout, true);
            return {};
        }
    }
}
