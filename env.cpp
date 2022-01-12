
#include "msl.h"

namespace MSL
{
    namespace
    {
        void fmtfunc_checkargs(std::string_view n, const Location& p, const std::vector<Value>& args)
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

        /*
        * this is not an exhaustive *printf impl. it only implements basic flags.
        *
        * apart from the classic ones, there are two 'special' ones:
        *
        *  '%v' calls Value::toStream()
        *  '%p' calls Value::reprToStream()
        *
        * tbd:
        *    '%<n>' (i.e., %1, %2, %3, ....) calls args[n].toStream()
        */
        template<typename CharT, typename StreamT>
        void format_handler(
            Environment& env,
            const Location& place,
            StreamT& os,
            const std::basic_string<CharT>& fmt,
            const std::vector<Value>& args,
            bool shouldflush
        )
        {
            int ch;
            int nch;
            size_t i;
            size_t argi;
            (void)env;
            ch = -1;
            nch = -1;
            argi = 0;
            for(i=0; i<fmt.size(); i++)
            {
                ch = fmt[i];
                nch = -1;
                if((i + 1) < fmt.size())
                {
                    nch = fmt[i + 1];
                }
                if(ch == '%')
                {
                    i++;
                    if(nch == '%')
                    {
                        os << '%';
                        if(shouldflush)
                        {
                            os << std::flush;
                        }
                    }
                    else
                    {
                        if(argi == args.size())
                        {
                            throw Error::ArgumentError(place, "format has more directives than arguments");
                        }
                        const auto& av = args[argi];
                        switch(nch)
                        {
                            case 'v':
                                {
                                    av.toStream(os);
                                }
                                break;
                            case 'p':
                                {
                                    av.reprToStream(os);
                                }
                                break;
                            case 's':
                                {
                                    os << av.getString();
                                    if(shouldflush)
                                    {
                                        os << std::flush;
                                    }
                                }
                                break;
                            case 'c':
                                {
                                    if(av.isNumber())
                                    {
                                        os << char(av.getNumber());
                                        if(shouldflush)
                                        {
                                            os << std::flush;
                                        }
                                    }
                                    else
                                    {
                                        throw Error::TypeError(place, "format directive expected number");
                                    }
                                }
                                break;
                            case 'd':
                            case 'f':
                            case 'g':
                                {
                                    if(av.isNumber())
                                    {
                                        os << av.getNumber();
                                        if(shouldflush)
                                        {
                                            os << std::flush;
                                        }
                                    }
                                    else
                                    {
                                        throw Error::TypeError(place, "format directive expected number");
                                    }
                                }
                                break;
                            default:
                                {
                                    throw Error::ArgumentError(place, "invalid format directive");
                                }
                                break;
                        }
                        argi++;
                    }
                }
                else
                {
                    os << char(ch);
                    if(shouldflush)
                    {
                        os << std::flush;
                    }
                }
            }
        }

        class IOHandle: public Object
        {
            public:
                FILE* m_stream;

            public:
                IOHandle(FILE* s): m_stream(s)
                {
                }

                Value io_write(AST::ExecutionContext& ctx, const Location& place, std::vector<Value>&& args)
                {
                    for(const auto& a: args)
                    {
                        auto tmpstr = a.toString();
                        std::fwrite(tmpstr.data(), sizeof(char), tmpstr.size(), m_stream);
                    }
                    return {};
                }

        };

        /*
        // "why C file handles :(((((("
        // because C++ streams are not inter-compatible, which is to say,
        // you can have a reference, perhaps even a pointer to say, a ostream.
        // but that interface (templated or not), *might* include code for istream, or if
        // you feel particularly self-destructive, fstreams, which *might* not have the same interface
        // for eachother.
        // tl;dr C++ streams are really, really terrible.
        // ironically, they're good for output, acceptable for input, but absolute HORSE SHIT for abstraction.
        // (btw i'm aware that most *stream classes have some sort of inherited base class, but
        // std::ios* is very much extremely useless. just read the docs. it's no use.)
        */
        void makeStdHandle(Environment& upenv, const Location& upplace, const std::string& name, FILE* strm)
        {
            auto obj = std::make_shared<IOHandle>(strm);
            /*
            // NP. do not capture obj by reference, since it is a shared_ptr, which are ref-counted.
            // capturing it by ref rather than value would not increment/decrement the references, and thus,
            // at the end of the scope of this function, obj would be gone, and none of this would work.
            // it's a really common oversight, especially since most compilers won't warn about it, and
            // i've yet to see a code checker that actually discovers this.
            // so many fun ways to accidentally ruin your day!
            */
            /// todo: abstract this noise away? somehow?
            obj->entry("write") = Value{[&, obj](AST::ExecutionContext& ctx, const Location& place, AST::ThisType&, std::vector<Value>&& args) -> Value
            {
                return obj->io_write(ctx, place, std::move(args));
            }};
            upenv.global(name) = Value{std::move(obj)};
        }
    }

    namespace AST
    {
        Value ConstantExpression::execute(ExecutionContext& ctx) const
        {
            /* Nothing - just ignore its value. */
            (void)ctx;
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

        Value func_sprintf(Environment& env, const Location& place, std::vector<Value>&& args)
        {
            std::stringstream ss;
            std::string fmtstr;
            fmtfunc_checkargs("sprintf", place, args);
            fmtstr = args[0].toString();
            auto restargs = std::vector<Value>(args.begin() + 1, args.end());
            #if 0
            {
                std::cerr << "fmtstr=\"" << fmtstr << "\"" << std::endl;
                std::cerr << "restargs(" << restargs.size() << "): " << std::endl;
                for(size_t i=0; i<restargs.size(); i++)
                {
                    std::cerr << "  restargs[" << i << "] = " << restargs[i].toString() << std::endl;
                }
            }
            #endif
            format_handler(env, place, ss, fmtstr, restargs, false);
            return Value{ss.str()};
        }

        Value func_printf(Environment& env, const Location& place, std::vector<Value>&& args)
        {
            std::string fmtstr;
            fmtfunc_checkargs("printf", place, args);
            fmtstr = args[0].toString();
            auto restargs = std::vector<Value>(args.begin() + 1, args.end());
            format_handler(env, place, std::cout, fmtstr, restargs, true);
            return {};
        }
    }


    ////////////////////////////////////////////////////////////////////////////////
    // Environment

    Environment::Environment() : m_implenv{ new EnvironmentPimpl{ *this, m_globalscope } }
    {
        makeStdHandle(*this, {}, "$stdout", stdout);
        makeStdHandle(*this, {}, "$stderr", stderr);

        global("min") = Value{Builtins::func_min};
        global("max") = Value{Builtins::func_max};
        global("typeOf") = Value{Builtins::func_typeof};
        global("print") = Value{Builtins::func_print};
        global("println") = Value{Builtins::func_println};
        global("sprintf") = Value{Builtins::func_sprintf};
        global("printf") = Value{Builtins::func_printf};
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

}


