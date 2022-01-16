
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
#include <boost/date_time/posix_time/posix_time.hpp>
#include "gc.h"

using namespace std;

// Collectable
Collectable::Collectable() : mMarked(false)
{
    Collector::GC.addObject(this);
}

Collectable::Collectable(Collectable const&) : mMarked(false)
{
    Collector::GC.addObject(this);
}

Collectable::~Collectable()
{
}

void Collectable::mark()
{
    if(!mMarked)
    {
        mMarked = true;
        markChildren();
    }
}

void Collectable::markChildren()
{
}

// Memory
Memory::Memory(int size) : mSize(size)
{
    mMemory = new unsigned char[size];
}

Memory::~Memory()
{
    delete[] mMemory;
}

unsigned char* Memory::get()
{
    return mMemory;
}

int Memory::size()
{
    return mSize;
}

// Collector
Collector Collector::GC;

void Collector::collect(bool verbose)
{   
    // Mark root objects
    for(ObjectSet::iterator it = mRoots.begin(); it != mRoots.end(); ++it)
    {
        (*it)->mark();
    }
    // Mark pinned objects
    for(PinnedSet::iterator it = mPinned.begin(); it != mPinned.end(); ++it)
    {
        (*it).first->mark();
    }
    sweep(verbose);
}

void Collector::addRoot(Collectable* root)
{
    mRoots.insert(root);
}

void Collector::removeRoot(Collectable* root)
{
    mRoots.erase(root);
}

void Collector::pin(Collectable* o)
{
    PinnedSet::iterator it = mPinned.find(o);
    if(it == mPinned.end())
    {
        mPinned.insert(make_pair(o, 1));
    }
    else
    {
        (*it).second++;
    }
}

void Collector::unpin(Collectable* o)
{
    PinnedSet::iterator it = mPinned.find(o);
    assert(it != mPinned.end());

    if(--((*it).second) == 0)
        mPinned.erase(it);
}

void Collector::addObject(Collectable* o)
{
    mHeap.insert(o);
}

void Collector::removeObject(Collectable* o)
{
    mHeap.erase(o);
}

void Collector::sweep(bool verbose)
{
    unsigned int live = 0;
    unsigned int dead = 0;
    unsigned int total = 0;
    vector<ObjectSet::iterator> erase;
    for(auto it = mHeap.begin(); it != mHeap.end(); ++it)
    {
        auto p = *it;
        total++;
        if(p->mMarked)
        {
            p->mMarked = false;
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
        mHeap.erase(*it);
        delete p;
    }
}

int Collector::live()
{
    return mHeap.size();
}

