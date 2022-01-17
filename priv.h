
#pragma once
#include "msl.h"

namespace MSL
{
    namespace Util
    {
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
                const Value::List& m_args;
                int m_currch;
                int m_nextch;
                size_t m_curridx;
                size_t m_argcnt;
                size_t m_fmtsize;
                size_t m_argi;

            public:
                StringFormatter(Environment& env, const Location& loc, std::string_view fmt, const Value::List& args):
                m_env(env), m_place(loc), m_fmtstr(fmt), m_args(args)
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
                                            os << av.string();
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
                                                os << char(av.number());
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
                                                os << av.number();
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
    }

    namespace Builtins
    {
        Value func_printf(Environment& env, const Location& loc, Value::List&& args);
        Value func_sprintf(Environment& env, const Location& loc, Value::List&& args);
        Value func_println(Environment& env, const Location& loc, Value::List&& args);
        Value func_print(Environment& env, const Location& loc, Value::List&& args);
        Value func_max(Environment& ctx, const Location& loc, Value::List&& args);
        Value func_min(Environment& ctx, const Location& loc, Value::List&& args);
        Value func_typeof(Environment& env, const Location& loc, Value::List&& args);
        Value func_eval(Environment& env, const Location& loc, Value::List&& args);
        Value func_load(Environment& env, const Location& loc, Value::List&& args);


        void makeFileNamespace(Environment& env);

        Value ctor_null(AST::ExecutionContext& ctx, const Location& loc, Value::List&& args);
        Value ctor_number(AST::ExecutionContext& ctx, const Location& loc, Value::List&& args);
        Value ctor_string(AST::ExecutionContext& ctx, const Location& loc, Value::List&& args);
        Value ctor_object(AST::ExecutionContext& ctx, const Location& loc, Value::List&& args);
        Value ctor_array(AST::ExecutionContext& ctx, const Location& loc, Value::List&& args);
        Value ctor_function(AST::ExecutionContext& ctx, const Location& loc, Value::List&& args);
        Value ctor_type(AST::ExecutionContext& ctx, const Location& loc, Value::List&& args);


        Value protofn_object_count(AST::ExecutionContext& ctx, const Location& loc, Value&& objVal);

        Value protofn_string_length(AST::ExecutionContext& ctx, const Location& loc, Value&& objVal);
        Value protofn_string_chars(AST::ExecutionContext& ctx, const Location& loc, Value&& objVal);
        Value protofn_string_stripleft(AST::ExecutionContext& ctx, const Location& loc, Value&& objVal);
        Value protofn_string_stripright(AST::ExecutionContext& ctx, const Location& loc, Value&& objVal);
        Value protofn_string_strip(AST::ExecutionContext& ctx, const Location& loc, Value&& objVal);

        Value memberfn_string_resize(AST::ExecutionContext& ctx, const Location& loc, AST::ThisType& th, Value::List&& args);
        Value memberfn_string_startswith(AST::ExecutionContext& ctx, const Location& loc, AST::ThisType& th, Value::List&& args);
        Value memberfn_string_endswith(AST::ExecutionContext& ctx, const Location& loc, AST::ThisType& th, Value::List&& args);
        Value memberfn_string_includes(AST::ExecutionContext& ctx, const Location& loc, AST::ThisType& th, Value::List&& args);
        Value memberfn_string_split(AST::ExecutionContext& ctx, const Location& loc, AST::ThisType& th, Value::List&& args);

        Value protofn_array_length(AST::ExecutionContext& ctx, const Location& loc, Value&& objVal);
        Value memberfn_array_push(AST::ExecutionContext& ctx, const Location& loc, AST::ThisType& th, Value::List&& args);
        Value memberfn_array_pop(AST::ExecutionContext& ctx, const Location& loc, AST::ThisType& th, Value::List&& args);
        Value memberfn_array_insert(AST::ExecutionContext& ctx, const Location& loc, AST::ThisType& th, Value::List&& args);
        Value memberfn_array_remove(AST::ExecutionContext& ctx, const Location& loc, AST::ThisType& th, Value::List&& args);
        Value memberfn_array_each(AST::ExecutionContext& ctx, const Location& loc, AST::ThisType& th, Value::List&& args);
        Value memberfn_array_map(AST::ExecutionContext& ctx, const Location& loc, AST::ThisType& th, Value::List&& args);

    }

    struct StandardObjectMemberFunc
    {
        const char* name;
        MemberMethodFunction func;
    };

    struct StandardObjectPropertyFunc
    {
        const char* name;
        MemberPropertyFunction func;
    };
}
