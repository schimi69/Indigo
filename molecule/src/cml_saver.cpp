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

#include "base_cpp/output.h"
#include "molecule/cml_saver.h"
#include "molecule/molecule.h"
#include "molecule/query_molecule.h"
#include "molecule/elements.h"
#include "base_cpp/locale_guard.h"
#include "tinyxml.h"

using namespace indigo;

IMPL_ERROR(CmlSaver, "CML saver");

CmlSaver::CmlSaver (Output &output) : _output(output)
{
   skip_cml_tag = false;
}


void CmlSaver::saveMolecule (Molecule &mol)
{
   _saveMolecule(mol, false);
}

void CmlSaver::saveQueryMolecule (QueryMolecule &mol)
{
   _saveMolecule(mol, true);
}

void CmlSaver::_saveMolecule (BaseMolecule &mol, bool query)
{
   LocaleGuard locale_guard;
   int i;
   AutoPtr<TiXmlDocument> doc(new TiXmlDocument());
   _doc = doc->GetDocument();
   _root = 0;
   TiXmlElement * elem = 0;

   if (!skip_cml_tag)
   {
      TiXmlDeclaration * decl = new TiXmlDeclaration("1.0", "", "");
      _doc->LinkEndChild(decl);
      _root = new TiXmlElement("cml");
      _doc->LinkEndChild(_root);
      elem = _root;
   }

   _addMoleculeElement(elem, mol, query);
 
   _addRgroups(elem, mol, query);

   TiXmlPrinter printer;
   _doc->Accept(&printer);
   _output.printf("%s", printer.CStr());
   doc.release();
}

void CmlSaver::_addMoleculeElement (TiXmlElement *elem, BaseMolecule &mol, bool query)
{
   int i;

   BaseMolecule *_mol = &mol;
   QueryMolecule *qmol = 0;

   if (query)
      qmol = (QueryMolecule *)(&mol);

   TiXmlElement * molecule = new TiXmlElement("molecule");
   if (elem == 0)
      _doc->LinkEndChild(molecule);
   else
      elem->LinkEndChild(molecule);

   if (_mol->name.ptr() != 0)
   {
      if (strchr(_mol->name.ptr(), '\"') != NULL)
         throw Error("can not save molecule with '\"' in title");
      molecule->SetAttribute("title", _mol->name.ptr());
   }

   bool have_xyz = BaseMolecule::hasCoord(*_mol);
   bool have_z = BaseMolecule::hasZCoord(*_mol);

   if (_mol->vertexCount() > 0)
   {
      TiXmlElement * atomarray = new TiXmlElement("atomArray");
      molecule->LinkEndChild(atomarray);

      for (i = _mol->vertexBegin(); i != _mol->vertexEnd(); i = _mol->vertexNext(i))
      {

         int atom_number = _mol->getAtomNumber(i);

         const char *atom_str;

         if (_mol->isRSite(i))
            atom_str = "R";
         else if (_mol->isPseudoAtom(i))
            atom_str = _mol->getPseudoAtom(i);
         else if (atom_number > 0)
            atom_str = Element::toString(atom_number);
         else if (qmol != 0)
         {
            QS_DEF(Array<int>, list);

            int query_atom_type;
            if ((query_atom_type = QueryMolecule::parseQueryAtom(*qmol, i, list)) != -1)
            {
               if (query_atom_type == QueryMolecule::QUERY_ATOM_LIST ||
                   query_atom_type == QueryMolecule::QUERY_ATOM_NOTLIST)
               {
                  atom_str = Element::toString(list[0]);
               }
            }
         }

         TiXmlElement * atom = new TiXmlElement("atom");
         atomarray->LinkEndChild(atom);

         QS_DEF(Array<char>, buf);
         ArrayOutput out(buf);
         out.printf("a%d", i);
         buf.push(0);
         atom->SetAttribute("id", buf.ptr());
         atom->SetAttribute("elementType", atom_str);

         if (_mol->getAtomIsotope(i) != 0)
         {
            atom->SetAttribute("isotope", _mol->getAtomIsotope(i));
            // for inchi-1 program which ignores "isotope" property (version 1.03)
            atom->SetAttribute("isotopeNumber", _mol->getAtomIsotope(i));
         }

         if (_mol->getAtomCharge(i) != 0)
            atom->SetAttribute("formalCharge", _mol->getAtomCharge(i));


         if (!_mol->isRSite(i) && !_mol->isPseudoAtom(i))
         {
            if (_mol->getAtomRadical_NoThrow(i, 0) > 0)
            {
               atom->SetAttribute("spinMultiplicity", _mol->getAtomRadical(i));
               if (_mol->getAtomRadical_NoThrow(i, 0) == 1)
                  atom->SetAttribute("radical", "divalent1");
               else if (_mol->getAtomRadical_NoThrow(i, 0) == 2)
                  atom->SetAttribute("radical", "monovalent");
               else if (_mol->getAtomRadical_NoThrow(i, 0) == 3)
                  atom->SetAttribute("radical", "divalent3");
            }

            if (_mol->getExplicitValence(i) > 0)
               atom->SetAttribute("mrvValence", _mol->getExplicitValence(i));

            if (qmol == 0)
            {
               if (Molecule::shouldWriteHCount(mol.asMolecule(), i))
               {
                  int hcount;
      
                  try
                  {
                     hcount = _mol->getAtomTotalH(i);
                  }
                  catch (Exception &)
                  {
                     hcount = -1;
                  }
      
                  if (hcount >= 0)
                     atom->SetAttribute("hydrogenCount", hcount);
               }
            }
         }

         if (_mol->isRSite(i))
         {
            QS_DEF(Array<int>, rg_refs);

            _mol->getAllowedRGroups(i, rg_refs);

            QS_DEF(Array<char>, buf);
            ArrayOutput out(buf);

            if (rg_refs.size() == 1)
            {
               out.printf("%d", rg_refs[0]);
               buf.push(0);
               atom->SetAttribute("rgroupRef", buf.ptr());
            }
         }

         if (qmol != 0)
         {
            QS_DEF(Array<char>, buf);
            ArrayOutput out(buf);

            QS_DEF(Array<int>, list);

            int query_atom_type;
            if ((query_atom_type = QueryMolecule::parseQueryAtom(*qmol, i, list)) != -1)
            {
               if (query_atom_type == QueryMolecule::QUERY_ATOM_LIST ||
                   query_atom_type == QueryMolecule::QUERY_ATOM_NOTLIST)
               {
                  int k;

                  out.writeString("L");
                 
                  for (k = 0; k < list.size(); k++)
                  {
                     if (query_atom_type == QueryMolecule::QUERY_ATOM_NOTLIST)
                        out.writeString("!");
                     else
                        out.writeString(",");

                     out.writeString(Element::toString(list[k]));
                  }
                  out.writeString(": ");
               }
            }

            int rbc;
            if (_getRingBondCountFlagValue(*qmol, i, rbc))
            {
               if (rbc > 0)
                  out.printf("rb%d;", rbc);
               else if (rbc == -2)
                  out.printf("rb*;");
               else if (rbc == -1)
                  out.printf("rb0;");
            }

            int subst;
            if (_getSubstitutionCountFlagValue(*qmol, i, subst))
            {
               if (subst > 0)
                  out.printf("s%d;", subst);
               else if (subst == -2)
                  out.printf("s*;");
               else if (subst == -1)
                  out.printf("s0;");
            }

            int unsat;
            if (qmol->getAtom(i).sureValue(QueryMolecule::ATOM_UNSATURATION, unsat))
               out.printf("u1");

            if (buf.size() > 0)
            {
               buf.push(0);
               atom->SetAttribute("mrvQueryProps", buf.ptr());
            }
         }

         if (_mol->attachmentPointCount() > 0)
         {
            int val = 0;
   
            for (int idx = 1; idx <= _mol->attachmentPointCount(); idx++)
            {
               for (int j = 0; _mol->getAttachmentPoint(idx, j) != -1; j++)
               {
                  if (_mol->getAttachmentPoint(idx, j) == i)
                  {
                     val |= 1 << (idx - 1);
                     break;
                  }
               }
            }
   
            if (val > 0)
            {
               if (val == 3)
                  atom->SetAttribute("attachmentPoint", "both");
               else
                  atom->SetAttribute("attachmentPoint", val);
            }
         }

         if (have_xyz)
         {
            Vec3f &pos = _mol->getAtomXyz(i);

            if (have_z)
            {
               atom->SetDoubleAttribute("x3", pos.x);
               atom->SetDoubleAttribute("y3", pos.y);
               atom->SetDoubleAttribute("z3", pos.z);
            }
            else
            {
               atom->SetDoubleAttribute("x2", pos.x);
               atom->SetDoubleAttribute("y2", pos.y);
            }
         }

         if (_mol->stereocenters.getType(i) > MoleculeStereocenters::ATOM_ANY)
         {
            TiXmlElement * atomparity = new TiXmlElement("atomParity");
            atom->LinkEndChild(atomparity);

            QS_DEF(Array<char>, buf);
            ArrayOutput out(buf);
            const int *pyramid = _mol->stereocenters.getPyramid(i);
            if (pyramid[3] == -1)
               out.printf("a%d a%d a%d a%d", pyramid[0], pyramid[1], pyramid[2], i);
             else
               out.printf("a%d a%d a%d a%d", pyramid[0], pyramid[1], pyramid[2], pyramid[3]);
            buf.push(0);
            atomparity->SetAttribute("atomRefs4", buf.ptr());

            atomparity->LinkEndChild(new TiXmlText("1"));
         }
      }

      int latest_ind = i;

      if (_mol->attachmentPointCount() > 0)
      {
         for (i = _mol->vertexBegin(); i != _mol->vertexEnd(); i = _mol->vertexNext(i))
         {
            int val = 0;
   
            for (int idx = 1; idx <= _mol->attachmentPointCount(); idx++)
            {
               for (int j = 0; _mol->getAttachmentPoint(idx, j) != -1; j++)
               {
                  if (_mol->getAttachmentPoint(idx, j) == i)
                  {
                     val |= 1 << (idx - 1);
                     break;
                  }
               }
            }
   
            if (val > 0)
            {
               TiXmlElement * atom = new TiXmlElement("atom");
               atomarray->LinkEndChild(atom);
   
               QS_DEF(Array<char>, buf);
               ArrayOutput out(buf);
               out.printf("a%d", latest_ind++);
               buf.push(0);
               atom->SetAttribute("id", buf.ptr());
               atom->SetAttribute("elementType", "*");

            }
         }
      }
   }

   if (_mol->edgeCount() > 0)
   {
      TiXmlElement * bondarray = new TiXmlElement("bondArray");
      molecule->LinkEndChild(bondarray);

      for (i = _mol->edgeBegin(); i != _mol->edgeEnd(); i = _mol->edgeNext(i))
      {
         const Edge &edge = _mol->getEdge(i);

         TiXmlElement * bond = new TiXmlElement("bond");
         bondarray->LinkEndChild(bond);

         QS_DEF(Array<char>, buf);
         ArrayOutput out(buf);
         out.printf("a%d a%d", edge.beg, edge.end);
         buf.push(0);
         bond->SetAttribute("atomRefs2", buf.ptr());


         int order = _mol->getBondOrder(i);

         if (order == BOND_SINGLE || order == BOND_DOUBLE || order == BOND_TRIPLE)
            bond->SetAttribute("order", order);
         else if (order == BOND_AROMATIC)
            bond->SetAttribute("order", "A");


         if (qmol != 0)
         {
            if (order < 0)
               bond->SetAttribute("order", 1);

            int qb = QueryMolecule::getQueryBondType(qmol->getBond(i));
   
            if (qb == QueryMolecule::QUERY_BOND_SINGLE_OR_DOUBLE)
               bond->SetAttribute("queryType", "SD");
            else if (qb == QueryMolecule::QUERY_BOND_SINGLE_OR_AROMATIC)
               bond->SetAttribute("queryType", "SA");
            else if (qb == QueryMolecule::QUERY_BOND_DOUBLE_OR_AROMATIC)
               bond->SetAttribute("queryType", "DA");
            else if (qb == QueryMolecule::QUERY_BOND_ANY)
               bond->SetAttribute("queryType", "Any");
         }

         int dir = _mol->getBondDirection(i);
         int parity = _mol->cis_trans.getParity(i);

         if (dir == BOND_UP || dir == BOND_DOWN)
         {
            TiXmlElement * bondstereo = new TiXmlElement("bondStereo");
            bond->LinkEndChild(bondstereo);
            bondstereo->LinkEndChild(new TiXmlText((dir == BOND_UP) ? "W" : "H"));
         }
         else if (parity != 0)
         {
            TiXmlElement * bondstereo = new TiXmlElement("bondStereo");
            bond->LinkEndChild(bondstereo);

            QS_DEF(Array<char>, buf);
            ArrayOutput out(buf);

            const int *subst = _mol->cis_trans.getSubstituents(i);
            out.printf("a%d a%d a%d a%d", subst[0], edge.beg, edge.end, subst[2]);
            buf.push(0);
            bondstereo->SetAttribute("atomRefs4", buf.ptr());
            bondstereo->LinkEndChild(new TiXmlText((parity == MoleculeCisTrans::CIS) ? "C" : "T"));
         }
      }
   }

   if (_mol->countSGroups() > 0)
   {
      MoleculeSGroups *sgroups = &_mol->sgroups;

      for (i = _mol->sgroups.begin(); i != _mol->sgroups.end(); i = _mol->sgroups.next(i))
      {
         SGroup &sgroup = sgroups->getSGroup(i);

         if (sgroup.parent_group == 0)
            _addSgroupElement(molecule, *_mol, sgroup);
      }
   }
}

void CmlSaver::_addSgroupElement (TiXmlElement *molecule, BaseMolecule &mol, SGroup &sgroup)
{
   TiXmlElement * sg = new TiXmlElement("molecule");
   molecule->LinkEndChild(sg);

   QS_DEF(Array<char>, buf);
   ArrayOutput out(buf);
   out.printf("sg%d", sgroup.original_group);
   buf.push(0);
   sg->SetAttribute("id", buf.ptr());

   if (sgroup.atoms.size() > 0)
   {
      QS_DEF(Array<char>, buf);
      ArrayOutput out(buf);

      for (int j = 0; j < sgroup.atoms.size(); j++)
         out.printf("a%d ", sgroup.atoms[j]);

      buf.pop();
      buf.push(0);

      sg->SetAttribute("atomRefs", buf.ptr());
   }

   if (sgroup.brackets.size() > 0)
   {
      TiXmlElement * brks = new TiXmlElement("MBracket");
      sg->LinkEndChild(brks);

      if (sgroup.brk_style == 0)
         brks->SetAttribute("type", "SQUARE");
      else
         brks->SetAttribute("type", "ROUND");

      for (int i = 0; i < sgroup.brackets.size(); i++)
      {
         TiXmlElement * pnt0 = new TiXmlElement("MPoint");
         brks->LinkEndChild(pnt0);
         pnt0->SetDoubleAttribute("x", sgroup.brackets[i][0].x);
         pnt0->SetDoubleAttribute("y", sgroup.brackets[i][0].y);

         TiXmlElement * pnt1 = new TiXmlElement("MPoint");
         brks->LinkEndChild(pnt1);
         pnt1->SetDoubleAttribute("x", sgroup.brackets[i][1].x);
         pnt1->SetDoubleAttribute("y", sgroup.brackets[i][1].y);
      }
   }

   if (sgroup.sgroup_type == SGroup::SG_TYPE_DAT)
   {
      sg->SetAttribute("role", "DataSgroup");

      DataSGroup &dsg = (DataSGroup &)sgroup;

      const char *name = dsg.name.ptr();
      if (name != 0 && strlen(name) > 0)
      {
         sg->SetAttribute("fieldName", name);
      }
      const char *desc = dsg.description.ptr();
      if (desc != 0 && strlen(desc) > 0)
      {
         sg->SetAttribute("fieldType", desc);
      }
      const char *querycode = dsg.querycode.ptr();
      if (querycode != 0 && strlen(querycode) > 0)
      {
         sg->SetAttribute("queryType", querycode);
      }
      const char *queryoper = dsg.queryoper.ptr();
      if (queryoper != 0 && strlen(queryoper) > 0)
      {
         sg->SetAttribute("queryOp", queryoper);
      }

      sg->SetDoubleAttribute("x", dsg.display_pos.x);
      sg->SetDoubleAttribute("y", dsg.display_pos.y);

      if (!dsg.detached)
      {
         sg->SetAttribute("dataDetached", "false");
      }

      if (dsg.relative)
      {
         sg->SetAttribute("placement", "Relative");
      }

      if (dsg.display_units)
      {
         sg->SetAttribute("unitsDisplayed", "Unit displayed");
      }

      char tag = dsg.tag;
      if (tag != 0 && tag != ' ')
      {
         sg->SetAttribute("tag", tag);
      }

      if (dsg.num_chars > 0)
      {
         sg->SetAttribute("displayedChars", dsg.num_chars);
      }

      if (dsg.data.size() > 0 && dsg.data[0] != 0)
      {
         sg->SetAttribute("fieldData", dsg.data.ptr());
      }

      MoleculeSGroups *sgroups = &mol.sgroups;
      
      for (int i = mol.sgroups.begin(); i != mol.sgroups.end(); i = mol.sgroups.next(i))
      {
         SGroup &sg_child = sgroups->getSGroup(i);

         if ( (sg_child.parent_group != 0) && (sg_child.parent_group == sgroup.original_group) )
            _addSgroupElement(sg, mol, sg_child);
      }
   }
   else if (sgroup.sgroup_type == SGroup::SG_TYPE_GEN)
   {
      sg->SetAttribute("role", "GenericSgroup");

      MoleculeSGroups *sgroups = &mol.sgroups;
      
      for (int i = mol.sgroups.begin(); i != mol.sgroups.end(); i = mol.sgroups.next(i))
      {
         SGroup &sg_child = sgroups->getSGroup(i);

         if ( (sg_child.parent_group != 0) && (sg_child.parent_group == sgroup.original_group) )
            _addSgroupElement(sg, mol, sg_child);
      }

   }
   else if (sgroup.sgroup_type == SGroup::SG_TYPE_SUP)
   {
      sg->SetAttribute("role", "SuperatomSgroup");

      Superatom &sup = (Superatom &)sgroup;

      const char *name = sup.subscript.ptr();
      if (name != 0 && strlen(name) > 0)
      {
         sg->SetAttribute("title", name);
      }

      MoleculeSGroups *sgroups = &mol.sgroups;
      
      for (int i = mol.sgroups.begin(); i != mol.sgroups.end(); i = mol.sgroups.next(i))
      {
         SGroup &sg_child = sgroups->getSGroup(i);

         if ( (sg_child.parent_group != 0) && (sg_child.parent_group == sgroup.original_group) )
            _addSgroupElement(sg, mol, sg_child);
      }
   }
   else if (sgroup.sgroup_type == SGroup::SG_TYPE_SRU)
   {
      sg->SetAttribute("role", "SruSgroup");

      RepeatingUnit &sru = (RepeatingUnit &)sgroup;

      const char *name = sru.subscript.ptr();
      if (name != 0 && strlen(name) > 0)
      {
         sg->SetAttribute("title", name);
      }

      if (sru.connectivity == SGroup::HEAD_TO_TAIL)
      {
         sg->SetAttribute("connect", "ht");
      }
      else if (sru.connectivity == SGroup::HEAD_TO_HEAD)
      {
         sg->SetAttribute("connect", "hh");
      }

      MoleculeSGroups *sgroups = &mol.sgroups;
      
      for (int i = mol.sgroups.begin(); i != mol.sgroups.end(); i = mol.sgroups.next(i))
      {
         SGroup &sg_child = sgroups->getSGroup(i);

         if ( (sg_child.parent_group != 0) && (sg_child.parent_group == sgroup.original_group) )
            _addSgroupElement(sg, mol, sg_child);
      }

   }
   else if (sgroup.sgroup_type == SGroup::SG_TYPE_MUL)
   {
      sg->SetAttribute("role", "MultipleSgroup");

      MultipleGroup &mul = (MultipleGroup &)sgroup;

      if (mul.multiplier > 0)
      {
         sg->SetAttribute("title", mul.multiplier);
      }

      if (mul.parent_atoms.size() > 0)
      {
         QS_DEF(Array<char>, buf);
         ArrayOutput out(buf);
   
         for (int j = 0; j < mul.parent_atoms.size(); j++)
            out.printf("a%d ", mul.parent_atoms[j]);
   
         buf.pop();
         buf.push(0);
   
         sg->SetAttribute("patoms", buf.ptr());
      }

      MoleculeSGroups *sgroups = &mol.sgroups;
      
      for (int i = mol.sgroups.begin(); i != mol.sgroups.end(); i = mol.sgroups.next(i))
      {
         SGroup &sg_child = sgroups->getSGroup(i);

         if ( (sg_child.parent_group != 0) && (sg_child.parent_group == sgroup.original_group) )
            _addSgroupElement(sg, mol, sg_child);
      }
   }
}

void CmlSaver::_addRgroups (TiXmlElement *elem, BaseMolecule &mol, bool query)
{
  if (mol.rgroups.getRGroupCount() > 0)
   {
      MoleculeRGroups &rgroups = mol.rgroups;
      int n_rgroups = rgroups.getRGroupCount();

      for (int i = 1; i <= n_rgroups; i++)
      {
         RGroup &rgroup = rgroups.getRGroup(i);

         if (rgroup.fragments.size() == 0)
            continue;

         TiXmlElement * rg = new TiXmlElement("Rgroup");
         elem->LinkEndChild(rg);

         rg->SetAttribute("rgroupID", i);


         if (rgroup.if_then > 0)
            rg->SetAttribute("thenR", rgroup.if_then);

         if (rgroup.rest_h > 0)
            rg->SetAttribute("restH", rgroup.rest_h);


         QS_DEF(Array<char>, buf);
         ArrayOutput out(buf);

         _writeOccurrenceRanges(out, rgroup.occurrence);

         if (buf.size() > 1)
            rg->SetAttribute("rlogicRange", buf.ptr());

         _addRgroupElement(rg, rgroup, query);
      }
   }
}

void CmlSaver::_addRgroupElement (TiXmlElement *elem, RGroup &rgroup, bool query)
{
   PtrPool<BaseMolecule> &frags = rgroup.fragments;

   for (int i = frags.begin(); i != frags.end(); i = frags.next(i))
   {
      BaseMolecule *fragment = frags[i];

      _addMoleculeElement(elem, *fragment, query);
   }
}

void CmlSaver::_writeOccurrenceRanges (Output &out, const Array<int> &occurrences)
{
   for (int i = 0; i < occurrences.size(); i++)
   {
      int occurrence = occurrences[i];

      if ((occurrence & 0xFFFF) == 0xFFFF)
         out.printf(">%d", (occurrence >> 16) - 1);
      else if ((occurrence >> 16) == (occurrence & 0xFFFF))
         out.printf("%d", occurrence >> 16);
      else if ((occurrence >> 16) == 0)
         out.printf("<%d", (occurrence & 0xFFFF) + 1);
      else
         out.printf("%d-%d", occurrence >> 16, occurrence & 0xFFFF);

      if (i != occurrences.size() - 1)
         out.printf(",");
   }
}

bool CmlSaver::_getRingBondCountFlagValue (QueryMolecule &qmol, int idx, int &value)
{
   QueryMolecule::Atom &atom = qmol.getAtom(idx);
   int rbc;
   if (atom.hasConstraint(QueryMolecule::ATOM_RING_BONDS))
   {
      if (atom.sureValue(QueryMolecule::ATOM_RING_BONDS, rbc))
      {
         value = rbc;
         if (value == 0)
            value = -1;
         return true;
      }
      int rbc_values[1] = { 4 };
      if (atom.sureValueBelongs(QueryMolecule::ATOM_RING_BONDS, rbc_values, 1))
      {
         value = 4;
         return true;
      }
   }
   else if (atom.sureValue(QueryMolecule::ATOM_RING_BONDS_AS_DRAWN, rbc))
   {
      value = -2;
      return true;
   }
   return false;
}

bool CmlSaver::_getSubstitutionCountFlagValue (QueryMolecule &qmol, int idx, int &value)
{
   QueryMolecule::Atom &atom = qmol.getAtom(idx);
   int v;
   if (atom.hasConstraint(QueryMolecule::ATOM_SUBSTITUENTS))
   {
      if (atom.sureValue(QueryMolecule::ATOM_SUBSTITUENTS, v))
      {
         value = v;
         if (value == 0)
            value = -1;
         return true;
      }
      int values[1] = { 6 };
      if (atom.sureValueBelongs(QueryMolecule::ATOM_SUBSTITUENTS, values, 1))
      {
         value = 6;
         return true;
      }
   }
   else if (atom.sureValue(QueryMolecule::ATOM_SUBSTITUENTS_AS_DRAWN, v))
   {
      value = -2;
      return true;
   }
   return false;
}
