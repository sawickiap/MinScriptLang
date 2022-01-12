
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
        class StringFormatter
        {
            public:

            private:
                Environment& m_env;
                const Location& m_place;
                
                std::string_view m_fmtstr;
                const std::vector<Value>& m_args;
                int m_currch;
                int m_nextch;
                size_t m_curridx;
                size_t m_argcnt;
                size_t m_fmtsize;
                size_t m_argi;

            public:
                StringFormatter(Environment& env, const Location& place, std::string_view fmt, const std::vector<Value>& args):
                m_env(env), m_place(place), m_fmtstr(fmt), m_args(args)
                {
                    (void)m_env;
                    m_currch = -1;
                    m_nextch = -1;
                    m_argi = 0;
                    m_argcnt = m_args.size();
                    m_fmtsize = m_fmtstr.size();
                }

                template<typename StreamT>
                void parseIndexed(StreamT& os)
                {
                    size_t start;
                    size_t aidx;
                    size_t thismuch;
                    if(std::isdigit(int(m_fmtstr[m_curridx+1])))
                    {
                        start = m_curridx+1;
                        thismuch = 0;
                        m_curridx++;
                        while(m_curridx < m_fmtsize)
                        {
                            if(m_fmtstr[m_curridx] == ')')
                            {
                                break;
                            }
                            if(!std::isdigit(int(m_fmtstr[m_curridx])))
                            {
                                //fprintf(stderr, "m_fmtstr[%d] = '%c'\n", m_curridx, m_fmtstr[m_curridx]);
                                throw Error::ArgumentError(m_place,
                                    "directive '%(...)' may only contain numbers (perhaps you forgot a ')')");
                            }
                            m_curridx++;
                            thismuch++;
                        }
                        if(m_curridx == m_fmtsize)
                        {
                            throw Error::ArgumentError(m_place, "directive '%(...)' missing closing ')'");
                        }
                        /*
                        std::cerr << "m_fmtstr(" << m_fmtsize << ") idx: start=" << start << ", end=" << thismuch << " = \"" << m_fmtstr.substr(start, thismuch) << "\"" << std::endl;
                        */
                        aidx = std::stoi(std::string(m_fmtstr.substr(start, thismuch)));
                        if(aidx > m_argcnt)
                        {
                            throw Error::ArgumentError(m_place,
                                Util::joinArgs("directive %(<n>) (where n=", aidx, ", args=", m_argcnt,") out of bounds"));
                        }
                        m_args[aidx].toStream(os);
                    }
                    else
                    {
                        throw Error::ArgumentError(m_place, "directive '%(...)' must start with a digit");
                    }
                }

                template<typename StreamT>
                void run(StreamT& os, bool shouldflush)
                {
                    for(m_curridx=0; m_curridx<m_fmtsize; m_curridx++)
                    {
                        m_currch = m_fmtstr[m_curridx];
                        m_nextch = -1;
                        if((m_curridx + 1) < m_fmtsize)
                        {
                            m_nextch = m_fmtstr[m_curridx + 1];
                        }
                        if(m_currch == '%')
                        {
                            m_curridx++;
                            if(m_nextch == '%')
                            {
                                os << '%';
                                if(shouldflush)
                                {
                                    os << std::flush;
                                }
                            }
                            else
                            {
                                if(m_argi == m_argcnt)
                                {
                                    throw Error::ArgumentError(m_place, "format has more directives than arguments");
                                }
                                const auto& av = m_args[m_argi];
                                switch(m_nextch)
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
                                                throw Error::TypeError(m_place, "format directive expected number");
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
                                                throw Error::TypeError(m_place, "format directive expected number");
                                            }
                                        }
                                        break;
                                    case '(':
                                        {
                                            parseIndexed(os);
                                        }
                                        break;
                                    default:
                                        {
                                            throw Error::ArgumentError(m_place, "invalid format directive");
                                        }
                                        break;
                                }
                                m_argi++;
                            }
                        }
                        else
                        {
                            os << char(m_currch);
                            if(shouldflush)
                            {
                                os << std::flush;
                            }
                        }
                    }
                }
        };

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
                    (void)ctx;
                    (void)place;
                    for(const auto& a: args)
                    {
                        auto tmpstr = a.toString();
                        std::fwrite(tmpstr.data(), sizeof(char), tmpstr.size(), m_stream);
                    }
                    return {};
                }

        };
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
            StringFormatter(env, place, fmtstr, restargs).run(ss, false);
            return Value{ss.str()};
        }

        Value func_printf(Environment& env, const Location& place, std::vector<Value>&& args)
        {
            std::string fmtstr;
            fmtfunc_checkargs("printf", place, args);
            fmtstr = args[0].toString();
            auto restargs = std::vector<Value>(args.begin() + 1, args.end());
            StringFormatter(env, place, fmtstr, restargs).run(std::cout, true);
            return {};
        }
    }


    ////////////////////////////////////////////////////////////////////////////////
    // Environment

    Environment::Environment() : m_implenv{ new EnvironmentPimpl{ *this, m_globalscope } }
    {
        #if 1
        makeStdHandle({}, "$stdout", stdout);
        makeStdHandle({}, "$stderr", stderr);
        #endif

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
        size_t i;
        i = 0;
        for(auto it=m_globalobjects.rbegin(); it!=m_globalobjects.rend(); it++)
        {
            fprintf(stderr, "resetting global object #%d ...\n", int(i));
            (*it).reset();
            i++;
        }
        delete m_implenv;
    }

    /*
    // here's how this works for now:
    // since stdout/stdin/etc are global pointers, we don't need to worry about them.
    // IOHandle, deriving Object, stores the pointer, and the member method is
    // a lambda that wraps the actual call to a shared_ptr instance of IOHandle.
    // this works fine, no leakage of any kind, buuuut...:
    // there is no real "constructor" (beyond what you see in makeStdHandle), and
    // no destructor, apart from whenever the shared_ptr gets destroyed.
    // and that's likely going to be a bit of an issue for other userdata, since with
    // this method, the object likely lives on beyond the scope of creation.
    //
    // as for "why C file handles :((((((" ...
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
    void Environment::makeStdHandle(const Location& upplace, const std::string& name, FILE* strm)
    {
        (void)upplace;
        auto obj = makeObject<IOHandle>(strm);
        auto weak = std::weak_ptr<IOHandle>(obj);
        /*
        // NP. do not capture obj by reference, since it is a shared_ptr, which are ref-counted.
        // capturing it by ref rather than value would not increment/decrement the references, and thus,
        // at the end of the scope of this function, obj would be gone, and none of this would work.
        // it's a really common oversight, especially since most compilers won't warn about it, and
        // i've yet to see a code checker that actually discovers this.
        // so many fun ways to accidentally ruin your day!
        */
        /// todo: abstract this noise away? somehow?
        obj->entry("write") = Value{[weak](AST::ExecutionContext& ctx, const Location& place, AST::ThisType&, std::vector<Value>&& args) -> Value
        {
            auto hereobj = weak.lock();
            return hereobj->io_write(ctx, place, std::move(args));
        }};
        global(name) = Value{obj};
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


