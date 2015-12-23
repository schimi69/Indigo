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

#include "base_cpp/array.h"
#include "molecule/molecule_attachments_search.h"

using namespace indigo;

IMPL_ERROR(Topology, "dependencies structure");
CP_DEF(Topology);

IMPL_ERROR(IntervalFilter, "occurrences interval filter");
CP_DEF(IntervalFilter);

IMPL_ERROR(GroupRestrictions, "restrictions on groups");
CP_DEF(GroupRestrictions);

IMPL_ERROR(OccurrenceRestrictions, "restrictions on groups occurences");
CP_DEF(OccurrenceRestrictions);

IMPL_ERROR(AttachmentsSearch, "site-group attachments search");
CP_DEF(AttachmentsSearch);

void MultiMap::insert(int key, int value) {
    Set* *ptr = map.at2(key);
    if (ptr != nullptr) {
        (*ptr)->insert(value);
        return;
    }

    Set *values = new Set();
    map.insert(key, values);
    values->insert(value);
}

void MultiMap::remove(int key, int value) {
    Set* *ptr = map.at2(key);
    if (ptr == nullptr) {
        return;
    }

    Set *values = (*ptr);
    values->remove_if_exists(value);
    if (values->size() == 0) {
        map.remove(key);
        delete values;
    }
}

void MultiMap::remove(int key) {
    if (map.find(key)) {
        map.remove(key);
    }
}

const Set& MultiMap::lookup(int key) const {
    Set* *ptr = map.at2(key);
    if (ptr == nullptr) return nil;
    return **ptr;
}

const char* MultiMap::print(const Set& set) {
    if (set.size() < 1) return "{}";

    char* ptr = new char[1024];
    const char* out = ptr;
    for (int i = set.begin(); i != set.end(); i = set.next(i)) {
        ptr += sprintf(ptr, "%s%d", i == set.begin() ? "{" : ", ", set.key(i));
    }
    ptr += sprintf(ptr, "}");
    return out;
}

const char* MultiMap::print(const Set* set) {
    if (set == nullptr) return "{}";
    return print(*set);
}

const char* MultiMap::print(bool d) const {
    if (map.size() < 1) return "[]";

    char* ptr = new char[1024];
    const char* out = ptr;

    const char* delim = d ? "->" : "<-";
    for (int i = map.begin(); i != map.end(); i = map.next(i)) {
        ptr += sprintf(ptr, "%s%d%s%s", i == map.begin() ? "[" : ", ",
            map.key(i), delim, print(map.value(i)));
    }
    ptr += sprintf(ptr, "]");
    return out;
}

const char* MultiMap::print() const {
    return print(true);
}

const char* Topology::print() const {
    const char* sf = forward.print(true);
    const char* sb = backward.print(false);
    const char* sc = MultiMap::print(&current);

    char* ptr = new char[strlen(sc) + strlen(sf) + strlen(sb) + 255];
    const char* out = ptr;
    ptr += sprintf(ptr, "<current  = %s;\n forward  = %s;\n backward = %s>", sc, sf, sb);
    return out;
}

const Path &Topology::history() const {
    Path *result = new Path();
    result->copy(path);
    return *result;
}

const Set& Topology::pending() const {
    return current;
}

void Topology::depends(Node source, Node target) {
    expand(std::max(source, target));

    if (path.size() > 0) {
        //throw Error("It is impossible to change topology"
        //    "after you started to resolve dependencies");
        printf("Error: It is impossible to change topology "
            "after you started to resolve dependencies\n");
        exit(-1);
        //todo
    }

    current.remove_if_exists(target);
    backward.insert(target, source);
    forward.insert(source, target);
}

bool Topology::satisfy(Node source) {
    if (!current.find(source)) {
        return false;
    }
    current.remove(source);

    const Set &targets = forward.lookup(source);
    for (int i = targets.begin(); i != targets.end(); i = targets.next(i)) {
        Node target = targets.key(i);
        backward.remove(target, source);
        if (backward.lookup(target).size() < 1) {
            current.insert(target);
        }
    }
    forward.remove(source);
    path.push(source);
    return true;
}

bool Topology::finished() const {
    const bool x = path.size() == lim;
    const bool y = current.size() < 1;
    assert(x || y ? x == y : true);
    return x && y;
}

void Topology::expand(int nlim) {
    for (int i = lim + 1; i <= nlim; i++) {
        current.insert(i);
    }
    lim = std::max(lim, nlim);
}

GroupRestrictions::GroupRestrictions(int groupsN, int sitesN) :
groups(groupsN), sites(sitesN),
CP_INIT
{
    restrictions.resize(sites);
    restrictions.fill(ANY);
};

void GroupRestrictions::free(Site site) {
    assert(1 <= site <= sites);
    restrictions[site] = ANY;
}
void GroupRestrictions::set(Site site, Group group){
    assert(1 <= site <= sites);
    assert(1 <= group <= groups);
    restrictions[site-1] = group;
}

bool GroupRestrictions::match(Site site, Group group) const {
    return restrictions[site] == ANY
        || restrictions[site] == group;
}

IntervalFilter::IntervalFilter(const Array<int> &array) :
CP_INIT
{
    map.copy(array);
    const int n = map.size();
    assert(n % 2 == 0);
    assert(n > 1);
}

IntervalFilter::IntervalFilter(const IntervalFilter &other) :
  IntervalFilter(other.map)
{ };

IntervalFilter::IntervalFilter(int from, int to) :
CP_INIT
{
    if (from > to) throw Error("Interval [%d..%d] is forbidden", from, to);
    map.push(from);
    map.push(to);
}

// + if the nearest point is on the right,
// - if on the left
int IntervalFilter::distance(int x) const {
    int x1_;
    for (int i = 0; i < map.size(); i += 2) {
        int x0 = map[i], x1 = map[i + 1]; x1_ = -x0;
        if (x1_ < x && x < x0) {
            const int l = x1_ - x, r = x0 - x;
            return -l < r ? l : r ;
        }
        if (x0 <= x && x <= x1) {
            return 0;
        }
        x1_ = x1;
    }
    assert(x1_ < x);
    return x1_ - x;
}

IntervalFilter IntervalFilter::join(const IntervalFilter &other) const {
    Array<int> merged;

    const Array<int> *xs = &map, *ys = &other.map;
    int n = xs->size(), m = ys->size();
    assert(n % 2 == 0 && m % 2 == 0);
    assert(n > 1 && m > 1);

    int    i = 0,   j = 0;
    while (i < n && j < m) {
        int x0 = (*xs)[i], x1 = (*xs)[i + 1],
            y0 = (*ys)[j], y1 = (*ys)[j + 1];
        assert(x0 <= x1);
        assert(y0 <= y1);
        swap5(xs, x0, x1, i, n,
              ys, y0, y1, j, m,
              y1 >= x1);
        if (x1 < y0) {
            merged.push(x0);
            merged.push(x1);
            i += 2;
        } else {
            merged.push(min4(x0, x1, y0, y1));
            merged.push(max4(x0, x1, y0, y1));
            i += 2; j += 2;
        }
    }
    if (i < n) { assert(j >= m); append(merged, *xs, i); }
    else { if (j < m) { append(merged, *ys, j); } }

    return IntervalFilter(merged);
}

void IntervalFilter::append(Array<int> &target, const Array<int> &source, int from) {
    const int n = source.size();
    assert((n - from) % 2 == 0);
    assert(from < n);

    for (int i = from; i < n; i++) {
        target.push(source[i]);
    }
}

const char* IntervalFilter::print() const {
    char* ptr = new char[1024];
    const char* out = ptr;

    bool empty = true;
    for (int i = 0; i < map.size(); i += 2) {
        const char* d = empty ? "" : " \\/ ";
        empty = false;

        int x0 = map[i], x1 = map[i+1];
        if (x0 == x1) { ptr += sprintf(ptr, "%s{%d}", d, x0); }
        else { ptr += sprintf(ptr, "%s[%d..%d]", d, x0, x1); };
    }

    return out;
}

const IntervalFilter OccurrenceRestrictions::DEFAULT = gt(0);

OccurrenceRestrictions::OccurrenceRestrictions(int groupsN) : groups(groupsN),
CP_INIT
{
    restrictions.resize(groups);
    restrictions.fill(DEFAULT);
}

void OccurrenceRestrictions::free(Group group) {
    set(group, DEFAULT);
}

void OccurrenceRestrictions::set(Group group, const IntervalFilter &f) {
    IntervalFilter& ff = restrictions[group - 1];
    restrictions[group-1] = f;
}

AttachmentsSearch::AttachmentsSearch() :
top(5), gr_rs(5,5), oc_rs(5),
CP_INIT
{
    //123
}

AttachmentsSearch::~AttachmentsSearch() {
}

void AttachmentsSearch::run() {

}