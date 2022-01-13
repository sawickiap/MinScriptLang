
#include "priv.h"

namespace MSL
{
    namespace
    {

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


    Environment::Environment() : m_implenv{ new EnvironmentPimpl{ *this, m_globalscope } }
    {
        makeStdHandle({}, "$stdin", stdin);
        makeStdHandle({}, "$stdout", stdout);
        makeStdHandle({}, "$stderr", stderr);
        global("min") = Value{Builtins::func_min};
        global("max") = Value{Builtins::func_max};
        global("typeOf") = Value{Builtins::func_typeof};
        global("print") = Value{Builtins::func_print};
        global("println") = Value{Builtins::func_println};
        global("sprintf") = Value{Builtins::func_sprintf};
        global("printf") = Value{Builtins::func_printf};
        Builtins::makeFileNamespace(*this);
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
        auto obj = std::make_shared<IOHandle>(strm);
        auto weak = std::weak_ptr<IOHandle>(obj);
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

    std::string_view Environment::getTypename(Value::Type type) const
    {
        return m_implenv->getTypename(type);
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

    std::string_view EnvironmentPimpl::getTypename(Value::Type type) const
    {
        //return VALUE_TYPE_NAMES[(size_t)type];
        // TODO support custom types
        return Value::getTypename(type);
    }

}


