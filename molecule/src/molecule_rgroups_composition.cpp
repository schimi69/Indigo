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
#include "base_cpp/auto_ptr.h"
#include "molecule/base_molecule.h"
#include "molecule/molecule_rgroups_composition.h"
#include "molecule/molecule_substructure_matcher.h"

using namespace indigo;

IMPL_ERROR(MoleculeRGroupsComposition, "molecule rgroups composition");

CP_DEF(MoleculeRGroupsComposition);

MoleculeRGroupsComposition::MoleculeRGroupsComposition () :
CP_INIT
{
}

MoleculeRGroupsComposition::~MoleculeRGroupsComposition () {
}

void MoleculeRGroupsComposition::convolve(BaseMolecule &source, BaseMolecule &target, int rgroup) {
    target.clone(source, nullptr, nullptr);
    convolve(target, rgroup);
}

void MoleculeRGroupsComposition::convolve(BaseMolecule &source, BaseMolecule &target) {
    target.clone(source, nullptr, nullptr);
    convolve(target);
}

void MoleculeRGroupsComposition::convolve(BaseMolecule &target, int idx) {
    //todo: handle 'occurrence', 'if_then', 'rest_h'
    //todo: which of combinations are required?
    //e.g. rgroup1 = {frag1, frag2}, rgroup2 = {frag3, frag4}
    //should result contain 12 combinations or only 2?

    MoleculeRGroups &rs = target.rgroups;

    auto k = rs.getRGroupCount();
    printf("%d rgroups\n", k);
    if (idx > k || idx < 1) {
        throw Error("rgroup index %d passed while there are %d rgroups (indices begin with 1)", idx, k);
    }

    auto n = target.countRSites();
    printf("%d rsites\n", n);

    RGroup &rgroup = rs.getRGroup(idx);
    auto   &fs = rgroup.fragments;
    auto m = fs.size();
    printf("%d fragments in rgroup #%d\n", m, idx);


    for (auto i = fs.begin(); i != fs.end(); i = fs.next(i)) {
        printf("%d ", i);
        printmol("fragment ", *fs[i], std);
    }

    //todo
    //convolve(<copy of target>, <fragments>)
    //put result into collection to retrieve them all at once later
}

void MoleculeRGroupsComposition::convolve(BaseMolecule &target) {
    //debug(target);
    //todo
    convolve(target, 1);
}

void MoleculeRGroupsComposition::convolve(BaseMolecule &target, PtrPool<BaseMolecule> fragments) {

}

// temporary {
FileOutput MoleculeRGroupsComposition::file("out");
StandardOutput MoleculeRGroupsComposition::std;
// } temporary