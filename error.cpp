
#include "msl.h"

namespace MSL
{
    ////////////////////////////////////////////////////////////////////////////////
    // Error implementation

    const char* Error::podMessage() const
    {
        if(m_what.empty())
            m_what = Format("(%u,%u): %.*s", m_place.textrow, m_place.textcolumn, (int)getMessage().length(), getMessage().data());
        return m_what.c_str();
    }

}

