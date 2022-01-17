
#include "priv.h"

namespace MSL
{
    namespace
    {
        class IOHandle: public Object
        {
            public:
                Environment& m_env;
                FILE* m_stream;

            public:
                IOHandle(Environment& env, FILE* s): m_env(env), m_stream(s)
                {
                }

                Value io_write(AST::ExecutionContext& ctx, const Location& loc, Value::List&& args)
                {
                    (void)ctx;
                    size_t rs;
                    rs = 0;
                    Util::ArgumentCheck ac(loc, args, "write");
                    ac.checkCount(1);
                    for(const auto& a: args)
                    {
                        auto tmpstr = a.toString();
                        rs += std::fwrite(tmpstr.data(), sizeof(char), tmpstr.size(), m_stream);
                        fflush(m_stream);
                    }
                    return Value{double(rs)};
                }

                Value io_getchar(AST::ExecutionContext& ctx, const Location& loc, Value::List&& args)
                {
                    int ch;
                    (void)ctx;
                    (void)args;
                    ch = fgetc(m_stream);
                    if(ch == EOF)
                    {
                        throw Error::EOFError(loc, "EOF");
                    }
                    fflush(m_stream);
                    return Value{double(ch)};
                }

                Value io_putchar(AST::ExecutionContext& ctx, const Location& loc, Value::List&& args)
                {
                    int rs;
                    Value ch;
                    (void)ctx;
                    Util::ArgumentCheck ac(loc, args, "putChar");
                    ch = ac.checkArgument(0, {Value::Type::Number});
                    rs = fputc(int(ch.number()), m_stream);
                    if(rs == EOF)
                    {
                        throw Error::EOFError(loc, "EOF");
                    }
                    fflush(m_stream);
                    return Value{double(rs)};
                }
        };
    }

    Environment::Environment() : m_implenv{ new EnvironmentPimpl{ *this, m_globalscope } }
    {
        makeStdHandle({}, "$stdin", stdin);
        makeStdHandle({}, "$stdout", stdout);
        makeStdHandle({}, "$stderr", stderr);
        setGlobal("eval", Value{Builtins::func_eval});
        setGlobal("load", Value{Builtins::func_load});
        setGlobal("min", Value{Builtins::func_min});
        setGlobal("max", Value{Builtins::func_max});
        setGlobal("typeOf", Value{Builtins::func_typeof});
        setGlobal("print", Value{Builtins::func_print});
        setGlobal("println", Value{Builtins::func_println});
        setGlobal("sprintf", Value{Builtins::func_sprintf});
        setGlobal("printf", Value{Builtins::func_printf});
        Builtins::makeFileNamespace(*this);
    }

    Environment::~Environment()
    {
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
        /// todo: abstract this noise away? somehow?
        #define do_entry(expname, classmethod) \
            {\
                obj->put(expname, Value{[weak](AST::ExecutionContext& ctx, const Location& loc, AST::ThisType&, Value::List&& args)\
                {\
                    auto hereobj = weak.lock(); \
                    return hereobj->classmethod(ctx, loc, std::move(args)); \
                }});\
            }
            
        (void)upplace;
        auto obj = std::make_shared<IOHandle>(*this, strm);
        auto weak = std::weak_ptr<IOHandle>(obj);
        {
            do_entry("write", io_write);
            do_entry("getChar", io_getchar);
            do_entry("putChar", io_putchar);
        }
        setGlobal(name, Value{obj});
    }

    Value Environment::execute(std::string_view code, std::string_view file)
    {
        return m_implenv->execute(code, file);
    }

    Value Environment::execute(std::string_view code)
    {
        return m_implenv->execute(code, "<script>");
    }


    std::string_view Environment::getTypename(Value::Type type) const
    {
        return m_implenv->getTypename(type);
    }

    void Environment::Print(std::string_view s)
    {
        m_implenv->Print(s);
    }

    Value EnvironmentPimpl::execute(std::string_view code, std::string_view filename)
    {
        AST::Script script{ Location{ 0, 1, 1, filename} };
        Tokenizer tokenizer{ code };
        Parser parser{ tokenizer };
        parser.parseScript(script);
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

    Value EnvironmentPimpl::execute(std::string_view code)
    {
        return execute(code, "<script>");
    }

    std::string_view EnvironmentPimpl::getTypename(Value::Type type) const
    {
        //return VALUE_TYPE_NAMES[(size_t)type];
        // TODO support custom types
        return Value::getTypename(type);
    }

}


