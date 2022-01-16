
#include "msl.h"

namespace MSL
{
    ////////////////////////////////////////////////////////////////////////////////
    // Error implementation
    namespace Error
    {
        std::string Exception::prettyMessage() const
        {
            if(m_what.empty())
            {
                m_what = name();
            }
            auto msg = message();
            return Util::formatString("(%u,%u): (%s) %.*s", m_place.textline, m_place.textcolumn, name().data(), int(msg.length()), msg.data());
        }
    }
}

