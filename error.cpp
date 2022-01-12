
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
            return Format("(%u,%u): (%s) %.*s", m_place.textline, m_place.textcolumn, name().data(), (int)getMessage().length(), getMessage().data());

        }
    }

}

