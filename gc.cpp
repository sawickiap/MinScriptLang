
// Copyright (C) 2009 Chris Double. All Rights Reserved.
// The original author of this code can be contacted at: chris.double@double.co.nz
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// DEVELOPERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//


#include <iostream>
#include "priv.h"

using namespace std;

namespace MSL
{
    namespace GC
    {
        // Collectable
        Collectable::Collectable() : m_marked(false)
        {
            Collector::GC.addObject(this);
        }

        Collectable::Collectable(Collectable const&) : m_marked(false)
        {
            Collector::GC.addObject(this);
        }

        Collectable::~Collectable()
        {
        }

        void Collectable::mark()
        {
            if(!m_marked)
            {
                m_marked = true;
                markChildren();
            }
        }

        void Collectable::markChildren()
        {
        }

        // Memory
        Memory::Memory(int size) : m_size(size)
        {
            m_memory = new unsigned char[size];
        }

        Memory::~Memory()
        {
            delete[] m_memory;
        }

        unsigned char* Memory::get()
        {
            return m_memory;
        }

        int Memory::size()
        {
            return m_size;
        }

        // Collector
        Collector Collector::GC;

        void Collector::collect(bool verbose)
        {   
            // Mark root objects
            for(ObjectSet::iterator it = m_roots.begin(); it != m_roots.end(); ++it)
            {
                (*it)->mark();
            }
            // Mark pinned objects
            for(PinnedSet::iterator it = m_pinned.begin(); it != m_pinned.end(); ++it)
            {
                (*it).first->mark();
            }
            sweep(verbose);
        }

        void Collector::addRoot(Collectable* root)
        {
            m_roots.insert(root);
        }

        void Collector::removeRoot(Collectable* root)
        {
            m_roots.erase(root);
        }

        void Collector::pin(Collectable* o)
        {
            auto it = m_pinned.find(o);
            if(it == m_pinned.end())
            {
                m_pinned.insert(make_pair(o, 1));
            }
            else
            {
                (*it).second++;
            }
        }

        void Collector::unpin(Collectable* o)
        {
            auto it = m_pinned.find(o);
            assert(it != m_pinned.end());

            if(--((*it).second) == 0)
                m_pinned.erase(it);
        }

        void Collector::addObject(Collectable* o)
        {
            m_heap.insert(o);
        }

        void Collector::removeObject(Collectable* o)
        {
            m_heap.erase(o);
        }

        void Collector::sweep(bool)
        {
            unsigned int live = 0;
            unsigned int dead = 0;
            unsigned int total = 0;
            std::vector<ObjectSet::iterator> erase;
            for(auto it = m_heap.begin(); it != m_heap.end(); ++it)
            {
                auto p = *it;
                total++;
                if(p->m_marked)
                {
                    p->m_marked = false;
                    ++live;
                }
                else
                {
                    erase.push_back(it);
                }
            }
            dead = erase.size();
            for(auto it = erase.begin(); it != erase.end(); ++it)
            {
                Collectable* p = **it;
                m_heap.erase(*it);
                delete p;
            }
        }

        int Collector::live()
        {
            return m_heap.size();
        }
    }
}

