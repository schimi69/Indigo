/****************************************************************************
 * Copyright (C) 2009-2015 EPAM Systems
 * 
 * This file is part of Indigo toolkit.
 * 
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ***************************************************************************/

#ifndef __molecule_attachments_search__
#define __molecule_attachments_search__

#include "assert.h"
#include "algorithm"

#include "base_cpp/array.h"
#include "base_cpp/red_black.h"
#include "base_cpp/exception.h"
#include "base_cpp/auto_ptr.h"
#include "base_cpp/tlscont.h"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
#endif

namespace indigo {

typedef RedBlackSet<int> Set;
typedef RedBlackMap<int, Set*> RawMultiMap;

class MultiMap {
public:
    explicit MultiMap() {}
    ~MultiMap() { /* todo */ }

    void insert(int key, int value);
    void remove(int key, int value);
    void remove(int key);

    const Set& lookup(int key) const;

    static const char* print(const Set& set);
    static const char* print(const Set* set);
           const char* print(bool d) const;
           const char* print() const;

protected:
    RawMultiMap map;
    const   Set nil;
};

typedef int   Node;
typedef Array<Node> Path;

class DLLEXPORT Topology {
public:
    explicit Topology(int size) :
    lim(0), CP_INIT {
        expand(size);
    }
    ~Topology() { /* todo */ }

    const Path &history() const;
    const Set  &pending() const;

    void depends(Node source, Node target);
    bool satisfy(Node source);

    bool finished() const;

    const char* print() const;

    DECL_ERROR;

protected:
    void expand(int nlim);

    MultiMap forward;
    MultiMap backward;
    Set      current;
    Path     path;
    int      lim;

    CP_DECL;
};

typedef int   Site;
typedef int   Group;
typedef Array<Group> Groups;

class DLLEXPORT GroupRestrictions {
public:
    GroupRestrictions(int groupsN, int sitesN);
    ~GroupRestrictions() { /* todo */ }

    void free(Site site);
    void set(Site site, Group group);
    bool match(Site site, Group group) const;

    const Group& operator[](Site site) const {
        return restrictions[site-1];
    }

    DECL_ERROR;

protected:
    static const Group ANY = 0;

    Groups restrictions;
    int groups, sites;

    CP_DECL;
};

#define swap1(l,r) \
    { auto t = l;  \
      l = r;       \
      r = t; }

#define swap5(ls,l0,l1,li,ln,rs,r0,r1,ri,rn,f) \
    if (!(f)) {           \
        printf("swap\n"); \
        swap1(ls,rs);     \
        swap1(l0,r0);     \
        swap1(l1,r1);     \
        swap1(li,ri);     \
        swap1(ln,rn);     \
    }

#define max4(w,x,y,z) std::max(std::max(w,x), std::max(y,z))
#define min4(w,x,y,z) std::min(std::min(w,x), std::min(y,z))

class DLLEXPORT IntervalFilter {
public:
    IntervalFilter(const IntervalFilter &other);
    IntervalFilter(int from, int to);
    ~IntervalFilter() { /* todo */ }

    int  distance(int x) const;
    bool match(int x)    const { return distance(x) == 0; }

    static const int MIN = 0;
    static const int MAX = INT_MAX;

    //possible to do this: IntervalFilter &f = range(3,5) && point(9) && gt(30)
    IntervalFilter operator&&(const IntervalFilter &other) const { return join(other); }
    IntervalFilter join(const       IntervalFilter &other) const;

    IntervalFilter& operator=(const IntervalFilter &other) {
        //todo: fix
        IntervalFilter *copy = new IntervalFilter(other);
        memcpy(this, copy, sizeof(*copy));
        return *this;
    }
    int operator[](int x) const { return distance(x); }

    const char* print() const;

    DECL_ERROR;

protected:
    explicit IntervalFilter(const Array<int> &array);

    Array<int> map; //todo: binary search is better

    CP_DECL;

private:
    static void append(Array<int> &target, const Array<int> &source, int from);
};

static IntervalFilter range(int l, int r) { return IntervalFilter(l, r); }
static IntervalFilter point(int n)        { return range(n, n); }
static IntervalFilter lt(int n)           { return range(INT_MIN, n - 1); }
static IntervalFilter gt(int n)           { return range(n + 1, INT_MAX); }

static IntervalFilter join(const IntervalFilter &a, const IntervalFilter &b) {
    return a.join(b);
}

typedef Array<IntervalFilter> IntervalFilters;

class DLLEXPORT OccurrenceRestrictions {
public:
    OccurrenceRestrictions(int groupsN);
    ~OccurrenceRestrictions() { /* todo */ }

    void free(Group site);
    void set(Group group, const IntervalFilter &f);

    const IntervalFilter& operator[](Group group) const {
        return restrictions[group - 1];
    }

    DECL_ERROR;

    static const IntervalFilter DEFAULT;

protected:

    IntervalFilters restrictions;
    int groups;

    CP_DECL;
};

class Attachment {
public:
    Attachment();
    ~Attachment();

protected:
    Array<int> sites; //-1 for H
};

class DLLEXPORT AttachmentsSearch {
public:
   AttachmentsSearch ();
   ~AttachmentsSearch ();

   void run();

   DECL_ERROR;

protected:
    const Topology               top;
    const GroupRestrictions      gr_rs;
    const OccurrenceRestrictions oc_rs;

    CP_DECL;

};

}

#ifdef _WIN32
#endif

#endif