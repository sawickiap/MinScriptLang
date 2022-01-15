/*/
via: https://gist.github.com/r-lyeh-archived/c653502b2918e92f082f

C++ Garbage Collection Library
==============================

This is a library to manage memory in C++ programs using a garbage
collector. It uses a mark and sweep algorithm.

All objects that are to be managed by the collector should be derived
from GCObject:

  class Test : public GCObject {
    ...
  };

If the object maintains references to other GC managed objects it
should override 'markChildren' to call 'mark' on those objects:

  class Test2 : public GCObject {
    private:
      Test* mTest;

    public:
      virtual void markChildren() {
        mTest->mark();
      }
    ...
  };

Periodic calls to GarbageCollector::GC.collect() should be made to
delete unreferenced objects and free memory. This call will call
'mark' on all the root objects, ensuring that they and their children
are not deleted, and and remaining objects are destroyed.

To add an object as a root, call GarbageCollector::GC.addRoot().

License
=======
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
DEVELOPERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Feedback
========
I host this on githib:
  http://github.com/doublec/gc

Feel free to clone, hack away, suggest patches, etc. I can be reached
via email: chris.double@double.co.nz
/*/

// Copyright (C) 2009 Chris Double. All Rights Reserved.
// See the license at the end of this file
#if !defined(GC_H)
#define GC_H

#include <set>
#include <map>

// Base class for all objects that are tracked by
// the garbage collector.
class GCObject {
 public:

  // For mark and sweep algorithm. When a GC occurs
  // all live objects are traversed and mMarked is
  // set to true. This is followed by the sweep phase
  // where all unmarked objects are deleted.
  bool mMarked;

 public:
  GCObject();
  GCObject(GCObject const&);
  virtual ~GCObject();

  // Mark the object and all its children as live
  void mark();

  // Overridden by derived classes to call mark()
  // on objects referenced by this object. The default
  // implemention does nothing.
  virtual void markChildren();
};

// Wrapper for an array of bytes managed by the garbage
// collector.
class GCMemory : public GCObject {
 public:
  unsigned char* mMemory;
  int   mSize;

 public:
  GCMemory(int size);
  virtual ~GCMemory();

  unsigned char* get();
  int size();
};

// Garbage Collector. Implements mark and sweep GC algorithm.
class GarbageCollector {
 public:
  // A collection of all active heap objects.
  typedef std::set<GCObject*> ObjectSet;
  ObjectSet mHeap;

  // Collection of objects that are scanned for garbage.
  ObjectSet mRoots;

  // Pinned objects
  typedef std::map<GCObject*, unsigned int> PinnedSet;
  PinnedSet mPinned;

  // Global garbage collector object
  static GarbageCollector GC;

 public:
  // Perform garbage collection. If 'verbose' is true then
  // GC stats will be printed to stdout.
  void collect(bool verbose = false);

  // Add a root object to the collector.
  void addRoot(GCObject* root);

  // Remove a root object from the collector.
  void removeRoot(GCObject* root);

  // Pin an object so it temporarily won't be collected. 
  // Pinned objects are reference counted. Pinning it
  // increments the count. Unpinning it decrements it. When
  // the count is zero then the object can be collected.
  void pin(GCObject* o);
  void unpin(GCObject* o);

  // Add an heap allocated object to the collector.
  void addObject(GCObject* o);

  // Remove a heap allocated object from the collector.
  void removeObject(GCObject* o);

  // Go through all objects in the heap, unmarking the live
  // objects and destroying the unreferenced ones.
  void sweep(bool verbose);

  // Number of live objects in heap
  int live();
};

#endif

#ifdef GC_IMPLEMENTATION

// Copyright (C) 2009 Chris Double. All Rights Reserved.
// See the license at the end of this file
#include <iostream>
#include <chrono>
#include <cassert>
#include <vector>

using namespace std;

// GCObject
GCObject::GCObject() :
  mMarked(false) {
  GarbageCollector::GC.addObject(this);
}

GCObject::GCObject(GCObject const&) :
  mMarked(false) {
  GarbageCollector::GC.addObject(this);
}

GCObject::~GCObject() {
}

void GCObject::mark() {
  if (!mMarked) {
    mMarked = true;
    markChildren();
  }
}

void GCObject::markChildren() {
}

// GCMemory
GCMemory::GCMemory(int size) : mSize(size) {
  mMemory = new unsigned char[size];
}

GCMemory::~GCMemory() {
  delete [] mMemory;
}

unsigned char* GCMemory::get() {
  return mMemory;
}

int GCMemory::size() {
  return mSize;
}

// GarbageCollector
GarbageCollector GarbageCollector::GC;

void GarbageCollector::collect(bool verbose) {
  using namespace std::chrono;
  static auto const epoch = steady_clock::now();
  unsigned int start = std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::steady_clock::now() - epoch ).count() / 1000000;

  // Mark root objects
  for (ObjectSet::iterator it = mRoots.begin();
       it != mRoots.end();
       ++it)
    (*it)->mark();

  // Mark pinned objects
  for (PinnedSet::iterator it = mPinned.begin();
       it != mPinned.end();
       ++it)
    (*it).first->mark();

  if (verbose) {
    cout << "Roots: " << mRoots.size() << endl;
    cout << "Pinned: " << mPinned.size() << endl;
    cout << "GC: " << mHeap.size() << " objects in heap" << endl;
  }

  sweep(verbose);

  if (verbose) {
    unsigned int end = std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::steady_clock::now() - epoch ).count() / 1000000;
    cout << "GC: " << (end-start) << " microseconds" << endl;
  }
}

void GarbageCollector::addRoot(GCObject* root) {
  mRoots.insert(root);
}

void GarbageCollector::removeRoot(GCObject* root) {
  mRoots.erase(root);
}

void GarbageCollector::pin(GCObject* o) {
  PinnedSet::iterator it = mPinned.find(o);
  if (it == mPinned.end()) {
    mPinned.insert(make_pair(o, 1));
  }
  else {
    (*it).second++;
  }
}

void GarbageCollector::unpin(GCObject* o) {
  PinnedSet::iterator it = mPinned.find(o);
  assert(it != mPinned.end());

  if (--((*it).second) == 0) 
    mPinned.erase(it);
}

void GarbageCollector::addObject(GCObject* o) {
  mHeap.insert(o);
}

void GarbageCollector::removeObject(GCObject* o) {
  mHeap.erase(o);
}

void GarbageCollector::sweep(bool verbose) {
  unsigned int live = 0;
  unsigned int dead = 0;
  unsigned int total = 0;
  vector<ObjectSet::iterator> erase;
  for (ObjectSet::iterator it = mHeap.begin();
       it != mHeap.end();
       ++it) {
    GCObject* p = *it;
    total++;
    if (p->mMarked) {
      p->mMarked = false;
      ++live;
    }
    else {
      erase.push_back(it);
    } 
  }
  dead = erase.size();
  for (vector<ObjectSet::iterator>::iterator it = erase.begin();
       it != erase.end();
       ++it) {
    GCObject* p = **it;
    mHeap.erase(*it);
    delete p;
  }
  if (verbose) {
    cout << "GC: " << live << " objects live after sweep" << endl;
    cout << "GC: " << dead << " objects dead after sweep" << endl;
  }
}

int GarbageCollector::live() {
  return mHeap.size();
}

#endif

#ifdef GC_SHOWCASE

// This is a library to manage memory in C++ programs using a garbage
// collector. It uses a mark and sweep algorithm.

// All objects that are to be managed by the collector should be derived
// from GCObject:

class Test : public GCObject {
};

// If the object maintains references to other GC managed objects it
// should override 'markChildren' to call 'mark' on those objects:

class Test2 : public GCObject {
private:
  Test* mTest;

public:
  virtual void markChildren() {
    mTest->mark();
  }
};

// Periodic calls to GarbageCollector::GC.collect() should be made to
// delete unreferenced objects and free memory. This call will call
// 'mark' on all the root objects, ensuring that they and their children
// are not deleted, and and remaining objects are destroyed.

// To add an object as a root, call GarbageCollector::GC.addRoot().

#include <vector>

int main() {
  {
    std::vector<Test *> k( 1000 );
    for( int i = 0; i < 1000; ++i ) {
      k[i] = new Test();
      k[i]->mark(); // mark obj as live
    }
    GarbageCollector::GC.collect(true);
  }

  GarbageCollector::GC.collect(true);
}

#endif
