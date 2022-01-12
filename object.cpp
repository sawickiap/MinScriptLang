
#include "msl.h"

namespace MSL
{
    Value* Object::tryGet(const std::string& key)
    {
        auto it = m_items.find(key);
        if(it != m_items.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    const Value* Object::tryGet(const std::string& key) const
    {
        auto it = m_items.find(key);
        if(it != m_items.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    bool Object::removeEntry(const std::string& key)
    {
        auto it = m_items.find(key);
        if(it != m_items.end())
        {
            m_items.erase(it);
            return true;
        }
        return false;
    }
}

