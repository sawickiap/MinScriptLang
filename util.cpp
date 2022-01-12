
#include "msl.h"

namespace MSL
{
    namespace Util
    {
        std::shared_ptr<Object> CopyObject(const Object& src)
        {
            auto dst = std::make_shared<Object>();
            for(const auto& item : src.m_items)
                dst->m_items.insert(item);
            return dst;
        }

        std::shared_ptr<Array> CopyArray(const Array& src)
        {
            auto dst = std::make_shared<Array>();
            const size_t count = src.m_items.size();
            dst->m_items.resize(count);
            for(size_t i = 0; i < count; ++i)
                dst->m_items[i] = Value{ src.m_items[i] };
            return dst;
        }

        bool NumberToIndex(size_t& outIndex, double number)
        {
            if(!std::isfinite(number) || number < 0.f)
                return false;
            outIndex = (size_t)number;
            return (double)outIndex == number;
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

    }
}

