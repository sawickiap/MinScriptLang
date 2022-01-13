
#include "msl.h"

namespace MSL
{
    Value* Object::tryGet(const std::string& key)
    {
        auto it = m_entrymap.find(key);
        if(it != m_entrymap.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    const Value* Object::tryGet(const std::string& key) const
    {
        auto it = m_entrymap.find(key);
        if(it != m_entrymap.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    bool Object::removeEntry(const std::string& key)
    {
        auto it = m_entrymap.find(key);
        if(it != m_entrymap.end())
        {
            m_entrymap.erase(it);
            return true;
        }
        return false;
    }
}

