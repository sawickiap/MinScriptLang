
#include "msl.h"

namespace MSL
{
    namespace Util
    {
        std::shared_ptr<Object> CopyObject(const Object& src)
        {
            auto dst = std::make_shared<Object>();
            for(const auto& item : src.m_entrymap)
                dst->m_entrymap.insert(item);
            return dst;
        }

        std::shared_ptr<Array> CopyArray(const Array& src)
        {
            auto dst = std::make_shared<Array>();
            const size_t count = src.m_arrayitems.size();
            dst->m_arrayitems.resize(count);
            for(size_t i = 0; i < count; ++i)
                dst->m_arrayitems[i] = Value{ src.m_arrayitems[i] };
            return dst;
        }

        bool NumberToIndex(size_t& outIndex, double number)
        {
            if(!std::isfinite(number) || number < 0.f)
            {
                return false;
            }
            outIndex = (size_t)number;
            return ((double)outIndex == number);
        }

        std::string VFormat(const char* format, va_list argList)
        {
            size_t dstlen;
            size_t rtlen;
            dstlen = 1024*1024;
            std::vector<char> buf(dstlen + 1);
            rtlen = vsnprintf(&buf[0], dstlen + 1, format, argList);
            return std::string{ &buf[0], rtlen };
        }

        std::string Format(const char* format, ...)
        {
            va_list argList;
            va_start(argList, format);
            auto result = VFormat(format, argList);
            va_end(argList);
            return result;
        }

        void checkArgumentCount(Environment& env, const Location& place, std::string_view fname, size_t argcnt, size_t expect)
        {
            (void)env;
            //if((!(expect > argcnt)) || ((expect > 0) && (argcnt == 0)))
            if((expect > 0) && ((argcnt == 0) || (argcnt < expect)))
            {
                throw Error::ArgumentError(place, Util::joinArgs("function '", fname, "' expects at least ", expect, " arguments"));
            }
        }

        Value checkArgument(Environment& env, const Location& place, std::string_view name, const Value::List& args, size_t idx, Value::Type type)
        {
            Value r;
            (void)env;
            checkArgumentCount(env, place, name, args.size(), idx);
            r = args[idx];
            // using Value::Type::Null means any type
            if((type != Value::Type::Null) && (r.type() != type))
            {
                auto etype = Value::getTypename(type);
                auto gtype = Value::getTypename(r.type());
                throw Error::TypeError(place,
                    Util::joinArgs("expected argument #", idx, " to be a ", etype, ", but got ", gtype, " instead"));
            }
            return r;
        }

    }
}

