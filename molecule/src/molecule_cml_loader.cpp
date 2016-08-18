/****************************************************************************
 * Copyright (C) 2011 EPAM Systems
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

#include "molecule/molecule_cml_loader.h"
#include "base_cpp/scanner.h"
#include "molecule/molecule.h"
#include "tinyxml.h"
#include "molecule/elements.h"
#include "molecule/molecule_scaffold_detection.h"

using namespace indigo;

IMPL_ERROR(MoleculeCmlLoader, "molecule CML loader");

MoleculeCmlLoader::MoleculeCmlLoader (Scanner &scanner)
{
   _scanner = &scanner;
   _handle = 0;
}

MoleculeCmlLoader::MoleculeCmlLoader (TiXmlHandle &handle)
{
   _handle = &handle;
   _scanner = 0;
}

void MoleculeCmlLoader::loadMolecule (Molecule &mol)
{
   mol.clear();

   if (_scanner != 0)
   {
      QS_DEF(Array<char>, buf);
      _scanner->readAll(buf);
      buf.push(0);
      TiXmlDocument xml;

      xml.Parse(buf.ptr());

      if (xml.Error())
         throw Error("XML parsing error: %s", xml.ErrorDesc());

      TiXmlHandle hxml(&xml);
      TiXmlNode *node;

      if (_findMolecule(hxml.ToNode()))
      {
         node = _molecule;
         TiXmlHandle molecule = _molecule;
         _loadMolecule(molecule, mol);

	 for (node = node->NextSibling();
              node != 0; 
              node = node->NextSibling())
         {
            if (strncmp(node->Value(), "Rgroup", 6) == 0)
            {
               TiXmlHandle rgroup = node;
               _loadRgroup(rgroup, mol);
            }
         }
      }
   }
   else
      _loadMolecule(*_handle, mol);

}

bool MoleculeCmlLoader::_findMolecule(TiXmlNode *elem)
{
	TiXmlNode *node;
   for (node = elem->FirstChild();
        node != 0; 
        node = node->NextSibling())
   {
      if (strncmp(node->Value(), "molecule", 8) == 0)
      {
         _molecule = node;
         return true;
      }
	  if (_findMolecule(node))
		  return true;
   }
   return false;
}

// Atoms
struct Atom
{
   std::string
      id, 
      element_type, 
      label, 
      isotope, 
      formal_charge,
      spin_multiplicity, 
      radical, 
      valence, 
      rgroupref, 
      attpoint, 
      attorder, 
      hydrogen_count, 
      x, y, z;
};

// This methods splits a space-separated string and writes each values into an arbitrary string
// property of Atom structure for each atom in the specified list
static void splitStringIntoProperties (const char *s, std::vector<Atom> &atoms, std::string Atom::*property)
{
   if (!s)
      return;

   std::stringstream stream(s);

   size_t i = 0;
   std::string token;
   while (stream >> token)
   {
      if (atoms.size() <= i)
         atoms.resize(i + 1);
      atoms[i].*property = token;
      i++;
   }
}

void MoleculeCmlLoader::_loadMolecule (TiXmlHandle &handle, Molecule &mol)
{
   std::unordered_map<std::string, int> atoms_id;
   std::unordered_map<std::string, size_t> atoms_id_int;

   // Function that mappes atom id as a string to an atom index
   auto getAtomIdx = [&](const char *id)
   {
      auto it = atoms_id.find(id);
      if (it == atoms_id.end())
         throw Error("atom id %s cannot be found", id);
      return it->second;
   };

   QS_DEF(Array<int>, total_h_count);
   atoms_id.clear();
   atoms_id_int.clear();
   total_h_count.clear();

   const char *title = handle.Element()->Attribute("title");

   if (title != 0)
      mol.name.readString(title, true);

   QS_DEF(std::vector<Atom>, atoms);
   atoms.clear();

   //
   // Read elements into an atoms array first and the parse them
   //

   TiXmlHandle atom_array = handle.FirstChild("atomArray");
   // Read atoms as xml attributes
   // <atomArray
   //       atomID="a1 a2 a3 ... "
   //       elementType="O C O ..."
   //       hydrogenCount="1 0 0 ..."
   // />
   splitStringIntoProperties(atom_array.Element()->Attribute("atomID"), atoms, &Atom::id);

   // Fill id if any were found
   size_t atom_index;
   for (atom_index = 0; atom_index < atoms.size(); atom_index++)
      atoms_id_int.emplace(atoms[atom_index].id, atom_index);

   splitStringIntoProperties(atom_array.Element()->Attribute("elementType"), atoms, &Atom::element_type);
   splitStringIntoProperties(atom_array.Element()->Attribute("mrvPseudo"), atoms, &Atom::label);
   splitStringIntoProperties(atom_array.Element()->Attribute("hydrogenCount"), atoms, &Atom::hydrogen_count);
   splitStringIntoProperties(atom_array.Element()->Attribute("x2"), atoms, &Atom::x);
   splitStringIntoProperties(atom_array.Element()->Attribute("y2"), atoms, &Atom::y);
   splitStringIntoProperties(atom_array.Element()->Attribute("x3"), atoms, &Atom::x);
   splitStringIntoProperties(atom_array.Element()->Attribute("y3"), atoms, &Atom::y);
   splitStringIntoProperties(atom_array.Element()->Attribute("z3"), atoms, &Atom::z);
   splitStringIntoProperties(atom_array.Element()->Attribute("isotope"), atoms, &Atom::isotope);
   splitStringIntoProperties(atom_array.Element()->Attribute("isotopeNumber"), atoms, &Atom::isotope);
   splitStringIntoProperties(atom_array.Element()->Attribute("formalCharge"), atoms, &Atom::formal_charge);
   splitStringIntoProperties(atom_array.Element()->Attribute("spinMultiplicity"), atoms, &Atom::spin_multiplicity);
   splitStringIntoProperties(atom_array.Element()->Attribute("radical"), atoms, &Atom::radical);
   splitStringIntoProperties(atom_array.Element()->Attribute("mrvValence"), atoms, &Atom::valence);
   splitStringIntoProperties(atom_array.Element()->Attribute("rgroupRef"), atoms, &Atom::rgroupref);
   splitStringIntoProperties(atom_array.Element()->Attribute("attachmentPoint"), atoms, &Atom::attpoint);
   splitStringIntoProperties(atom_array.Element()->Attribute("attachmentOrder"), atoms, &Atom::attorder);

   // Read atoms as nested xml elements
   //   <atomArray>
   //     <atom id="a1" elementType="H" />
   for (TiXmlElement *elem = atom_array.FirstChild().Element();
      elem; 
      elem = elem->NextSiblingElement())
   {
      if (strncmp(elem->Value(), "atom", 4) != 0)
         continue;

      const char *id = elem->Attribute("id");
      if (id == 0)
         throw Error("atom without an id");

      // Check if this atom has already been parsed
      auto it = atoms_id_int.find(id);
      if (it != end(atoms_id_int))
         atom_index = it->second;
      else
      {
         atom_index = atoms.size();
         atoms.emplace_back();
         atoms_id_int.emplace(id, atom_index);
      }
         
      Atom &a = atoms[atom_index];

      a.id = id;

      const char *element_type = elem->Attribute("elementType");

      if (element_type == 0)
         throw Error("atom without an elementType");
      a.element_type = element_type;

      const char *pseudo = elem->Attribute("mrvPseudo");

      if (pseudo != 0)
         a.label = pseudo;


      const char *isotope = elem->Attribute("isotope");

      if (isotope == 0)
         isotope = elem->Attribute("isotopeNumber");

      if (isotope != 0)
         a.isotope = isotope;

      const char *charge = elem->Attribute("formalCharge");

      if (charge != 0)
         a.formal_charge = charge;

      const char *spinmultiplicity = elem->Attribute("spinMultiplicity");
      
      if (spinmultiplicity != 0)
         a.spin_multiplicity = spinmultiplicity;

      const char *radical = elem->Attribute("radical");
      
      if (radical != 0)
         a.radical = radical;

      const char *valence = elem->Attribute("mrvValence");

      if (valence != 0)
         a.valence = valence;

      const char *hcount = elem->Attribute("hydrogenCount");

      if (hcount != 0)
         a.hydrogen_count = hcount;

      const char *x2 = elem->Attribute("x2");
      const char *y2 = elem->Attribute("y2");

      if (x2 != 0 && y2 != 0)
      {
         a.x = x2;
         a.y = y2;
      }

      const char *x3 = elem->Attribute("x3");
      const char *y3 = elem->Attribute("y3");
      const char *z3 = elem->Attribute("z3");

      if (x3 != 0 && y3 != 0 && z3 != 0)
      {
         a.x = x3;
         a.y = y3;
         a.z = z3;
      }

      const char *rgroupref = elem->Attribute("rgroupRef");

      if (rgroupref != 0)
         a.rgroupref = rgroupref;

      const char *attpoint = elem->Attribute("attachmentPoint");

      if (attpoint != 0)
         a.attpoint = attpoint;

      const char *attorder = elem->Attribute("attachmentOrder");

      if (attorder != 0)
         a.attorder = attorder;

   }

   // Parse them
   for (auto &a : atoms)
   {
      int label = Element::fromString2(a.element_type.c_str());


      if ( (label == -1) && (strncmp(a.element_type.c_str(), "R", 1) == 0) )
      {
         label = ELEM_RSITE;
      }
      else if ( (label == -1) && (strncmp(a.element_type.c_str(), "*", 1) == 0) )
      {
         label = ELEM_ATTPOINT;
      }
      else if ( (label == -1) || (!a.label.empty()) )
         label = ELEM_PSEUDO;

      int idx;
      if (label == ELEM_ATTPOINT)      
      {
         atoms_id.emplace(a.id, -1);
      }
      else
      {
         int idx = mol.addAtom(label);
   
         if (label == ELEM_PSEUDO)
         {
            if (!a.label.empty())
               mol.setPseudoAtom(idx, a.label.c_str());
            else
               mol.setPseudoAtom(idx, a.element_type.c_str());
         }

         total_h_count.expandFill(idx + 1, -1);
   
         atoms_id.emplace(a.id, idx);
   
         if (!a.isotope.empty())
         {
            int val;
            if (sscanf(a.isotope.c_str(), "%d", &val) != 1)
               throw Error("error parsing isotope");
            mol.setAtomIsotope(idx, val);
         }
   
         if (!a.formal_charge.empty())
         {
            int val;
            if (sscanf(a.formal_charge.c_str(), "%d", &val) != 1)
               throw Error("error parsing charge");
            mol.setAtomCharge(idx, val);
         }
   
         if (!a.spin_multiplicity.empty())
         {
            int val;
            if (sscanf(a.spin_multiplicity.c_str(), "%d", &val) != 1)
               throw Error("error parsing spin multiplicity");
            mol.setAtomRadical(idx, val);
         }
   
         if (!a.radical.empty())
         {
            int val = 0;
            if (strncmp(a.radical.c_str(), "divalent1", 9) == 0)
               val = 1;
            else if (strncmp(a.radical.c_str(), "monovalent", 10) == 0)
               val = 2;
            else if ( (strncmp(a.radical.c_str(), "divalent3", 9) == 0) ||
                      (strncmp(a.radical.c_str(), "divalent", 8) == 0)  ||
                      (strncmp(a.radical.c_str(), "triplet", 7) == 0) )
               val = 3;
            mol.setAtomRadical(idx, val);
         }

         if (a.spin_multiplicity.empty() && a.radical.empty())
         {
            mol.setAtomRadical(idx, 0);
         }

   
         if (!a.valence.empty())
         {
            int val;
            if (sscanf(a.valence.c_str(), "%d", &val) == 1)
               mol.setExplicitValence(idx, val);
         }
   
         if (!a.hydrogen_count.empty())
         {
            int val;
            if (sscanf(a.hydrogen_count.c_str(), "%d", &val) != 1)
               throw Error("error parsing hydrogen count");
            if (val < 0)
               throw Error("negative hydrogen count");
            total_h_count[idx] = val;
         }
   
         if (!a.x.empty())
            if (sscanf(a.x.c_str(), "%f", &mol.getAtomXyz(idx).x) != 1)
               throw Error("error parsing x");
   
         if (!a.y.empty())
            if (sscanf(a.y.c_str(), "%f", &mol.getAtomXyz(idx).y) != 1)
               throw Error("error parsing y");
   
         if (!a.z.empty())
            if (sscanf(a.z.c_str(), "%f", &mol.getAtomXyz(idx).z) != 1)
               throw Error("error parsing z");
   
         if (!a.rgroupref.empty())
         {
            int val;
            if (sscanf(a.rgroupref.c_str(), "%d", &val) != 1)
               throw Error("error parsing R-group reference");
            mol.allowRGroupOnRSite(idx, val);
         }
   
         if (!a.attpoint.empty())
         {
            int val;
            if (strncmp(a.attpoint.c_str(), "both", 4) == 0)
               val = 3;
            else if (sscanf(a.attpoint.c_str(), "%d", &val) != 1)
               throw Error("error parsing Attachment point");
            for (int att_idx = 0; (1 << att_idx) <= val; att_idx++)
               if (val & (1 << att_idx))
                  mol.addAttachmentPoint(att_idx + 1, idx);
         }
      }
/*
      if (!a.attorder.empty())
      {
         int val;
         if (sscanf(a.attorder.c_str(), "%d", &val) != 1)
            throw Error("error parsing Attachment order");
         mol.setRSiteAttachmentOrder(idx, nei_idx - 1, val - 1);
      }
*/
   }

   // Bonds
   bool have_cistrans_notation = false;

   for (TiXmlElement *elem = handle.FirstChild("bondArray").FirstChild().Element();
      elem; 
      elem = elem->NextSiblingElement())
   {
      if (strncmp(elem->Value(), "bond", 4) != 0)
         continue;
      const char *atom_refs = elem->Attribute("atomRefs2");
      if (atom_refs == 0)
         throw Error("bond without atomRefs2");

      BufferScanner strscan(atom_refs);
      QS_DEF(Array<char>, id);

      strscan.readWord(id, 0);
      int beg = getAtomIdx(id.ptr());
      strscan.skipSpace();
      strscan.readWord(id, 0);
      int end = getAtomIdx(id.ptr());

      if ((beg < 0) || (end < 0))
         continue;

      const char *order = elem->Attribute("order");
      if (order == 0)
         throw Error("bond without an order");

      int order_val;
      {
         if (order[0] == 'A' && order[1] == 0)
            order_val = BOND_AROMATIC;
         else if (order[0] == 'S' && order[1] == 0)
            order_val = BOND_SINGLE;
         else if (order[0] == 'D' && order[1] == 0)
            order_val = BOND_DOUBLE;
         else if (order[0] == 'T' && order[1] == 0)
            order_val = BOND_TRIPLE;
         else if (sscanf(order, "%d", &order_val) != 1)
            throw Error("error parsing order");
      }

      int idx = mol.addBond_Silent(beg, end, order_val);

      int dir = 0;

      TiXmlElement *bs_elem = elem->FirstChildElement("bondStereo");

      if (bs_elem != 0)
      {
         const char *text = bs_elem->GetText();
         if (text != 0)
         {
            if (text[0] == 'W' && text[1] == 0)
               dir = BOND_UP;
            if (text[0] == 'H' && text[1] == 0)
               dir = BOND_DOWN;
            if ((text[0] == 'C' || text[0] == 'T') && text[1] == 0)
               have_cistrans_notation = true;
         }
      }

      if (dir != 0)
         mol.setBondDirection(idx, dir);
   }

   // Implicit H counts
   int i, j;

   for (i = mol.vertexBegin(); i != mol.vertexEnd(); i = mol.vertexNext(i))
   {
      int h = total_h_count[i];

      if (h < 0)
         continue;

      const Vertex &vertex = mol.getVertex(i);

      for (j = vertex.neiBegin(); j != vertex.neiEnd(); j = vertex.neiNext(j))
         if (mol.getAtomNumber(vertex.neiVertex(j)) == ELEM_H)
            h--;

      if (h < 0)
         throw Error("hydrogenCount on atom %d is less than the number of explicit hydrogens");

      mol.setImplicitH(i, h);
   }

   // Tetrahedral stereocenters
   for (TiXmlElement *elem = handle.FirstChild("atomArray").FirstChild().Element(); 
      elem; 
      elem = elem->NextSiblingElement())
   {
      const char *id = elem->Attribute("id");
      
      if (id == 0)
         throw Error("atom without an id");

      int idx = getAtomIdx(id);

      TiXmlElement *ap_elem = elem->FirstChildElement("atomParity");

      if (ap_elem == 0)
         continue;

      const char *ap_text = ap_elem->GetText();

      if (ap_text == 0)
         continue;

      int val;
      if (sscanf(ap_text, "%d", &val) != 1)
         throw Error("error parsing atomParity");

      const char *refs4 = ap_elem->Attribute("atomRefs4");

      if (refs4 != 0)
      {
         BufferScanner strscan(refs4);
         QS_DEF(Array<char>, id);
         int k, pyramid[4];

         for (k = 0; k < 4; k++)
         {
            strscan.skipSpace();
            strscan.readWord(id, 0);
            pyramid[k] = getAtomIdx(id.ptr());
            if (pyramid[k] == idx)
               pyramid[k] = -1;
         }

         if (val < 0)
            __swap(pyramid[0], pyramid[1], k);

         MoleculeStereocenters::moveMinimalToEnd(pyramid);

         mol.stereocenters.add(idx, MoleculeStereocenters::ATOM_ABS, 0, pyramid);
      }
   }

   if (mol.stereocenters.size() == 0 && BaseMolecule::hasCoord(mol))
   {
      QS_DEF(Array<int>, sensible_bond_orientations);

      sensible_bond_orientations.clear_resize(mol.vertexEnd());
      mol.stereocenters.buildFromBonds(stereochemistry_options, sensible_bond_orientations.ptr());

      if (!stereochemistry_options.ignore_errors)
         for (i = 0; i < mol.vertexCount(); i++)
            if (mol.getBondDirection(i) > 0 && !sensible_bond_orientations[i])
               throw Error("direction of bond #%d makes no sense", i);
   }

   // Cis-trans stuff
   if (have_cistrans_notation)
   {
      int bond_idx = -1;

      for (TiXmlElement *elem = handle.FirstChild("bondArray").FirstChild().Element(); 
         elem; 
         elem = elem->NextSiblingElement())
      {
         if (strncmp(elem->Value(), "bond", 4) != 0)
            continue;
         bond_idx++;
         TiXmlElement *bs_elem = elem->FirstChildElement("bondStereo");

         if (bs_elem == 0)
            continue;

         const char *text = bs_elem->GetText();

         if (text == 0)
            continue;

         int parity;

         if (text[0] == 'C' && text[1] == 0)
            parity = MoleculeCisTrans::CIS;
         else if (text[0] == 'T' && text[1] == 0)
            parity = MoleculeCisTrans::TRANS;
         else
            continue;

         const char *atom_refs = bs_elem->Attribute("atomRefs4");
         // If there are only one substituent then atomRefs4 cano be omitted
         bool has_subst = atom_refs != nullptr;

         int substituents[4];

         if (!MoleculeCisTrans::isGeomStereoBond(mol, bond_idx, substituents, false))
            throw Error("cis-trans notation on a non cis-trans bond #%d", bond_idx);

         if (!MoleculeCisTrans::sortSubstituents(mol, substituents, 0))
            throw Error("cis-trans notation on a non cis-trans bond #%d", bond_idx);

         if (has_subst)
         {
            BufferScanner strscan(atom_refs);
            QS_DEF(Array<char>, id);
            int refs[4] = { -1, -1, -1, -1 };

            for (int k = 0; k < 4; k++)
            {
               strscan.skipSpace();
               strscan.readWord(id, 0);

               refs[k] = getAtomIdx(id.ptr());
            }

            const Edge &edge = mol.getEdge(bond_idx);

            if (refs[1] == edge.beg && refs[2] == edge.end)
               ;
            else if (refs[1] == edge.end && refs[2] == edge.beg)
            {
               std::swap(refs[0], refs[3]);
               std::swap(refs[1], refs[2]);
            }
            else
               throw Error("atomRefs4 in bondStereo do not match the bond ends");

            bool invert = false;
   
            if (refs[0] == substituents[0])
               ;
            else if (refs[0] == substituents[1])
               invert = !invert;
            else
               throw Error("atomRefs4 in bondStereo do not match the substituents");

            if (refs[3] == substituents[2])
               ;
            else if (refs[3] == substituents[3])
               invert = !invert;
            else
               throw Error("atomRefs4 in bondStereo do not match the substituents");

            if (invert)
               parity = 3 - parity;
         }

         mol.cis_trans.add(bond_idx, substituents, parity);
      }
   }
   else if (BaseMolecule::hasCoord(mol))
      mol.cis_trans.build(0);

   // Sgroups

   for (TiXmlElement *elem = handle.FirstChild().Element();
      elem; 
      elem = elem->NextSiblingElement())
   {
      if (strncmp(elem->Value(), "molecule", 8) != 0)
            continue;
      _loadSGroup(elem, mol, atoms_id, 0);
   }
}

void MoleculeCmlLoader::_loadSGroup (TiXmlElement *elem, Molecule &mol,
     std::unordered_map<std::string, int> &atoms_id, int sg_parent)
{
   auto getAtomIdx = [&](const char *id)
   {
      auto it = atoms_id.find(id);
      if (it == atoms_id.end())
         throw Error("atom id %s cannot be found", id);
      return it->second;
   };

   MoleculeSGroups *sgroups = &mol.sgroups;

   DataSGroup *dsg = 0;
   SGroup *gen = 0;
   RepeatingUnit *sru = 0;
   MultipleGroup *mul = 0;
   Superatom *sup = 0;

   int idx = 0;
   if (elem != 0)
   {
      const char *role = elem->Attribute("role");
      if (role == 0)
         throw Error("Sgroup without type");

      if (strncmp(role, "DataSgroup", 10) == 0)
      {
         idx = sgroups->addSGroup(SGroup::SG_TYPE_DAT);
         dsg = (DataSGroup *) &sgroups->getSGroup(idx);
      }
      else if (strncmp(role, "GenericSgroup", 13) == 0)
      {
         idx = sgroups->addSGroup(SGroup::SG_TYPE_GEN);
         gen = (SGroup *) &sgroups->getSGroup(idx);
      }
      else if (strncmp(role, "SruSgroup", 9) == 0)
      {
         idx = sgroups->addSGroup(SGroup::SG_TYPE_SRU);
         sru = (RepeatingUnit *) &sgroups->getSGroup(idx);
      }
      else if (strncmp(role, "MultipleSgroup", 14) == 0)
      {
         idx = sgroups->addSGroup(SGroup::SG_TYPE_MUL);
         mul = (MultipleGroup *) &sgroups->getSGroup(idx);
      }
      else if (strncmp(role, "SuperatomSgroup", 15) == 0)
      {
         idx = sgroups->addSGroup(SGroup::SG_TYPE_SUP);
         sup = (Superatom *) &sgroups->getSGroup(idx);
      }

      if ( (dsg == 0) && (gen == 0) && (sru == 0) && (mul == 0) && (sup == 0) )
         return;

      if (dsg != 0)
      {
         if (sg_parent > 0)
            dsg->parent_group = sg_parent;
   
         const char *atom_refs = elem->Attribute("atomRefs");
         if (atom_refs != 0)
         {
            BufferScanner strscan(atom_refs);
            QS_DEF(Array<char>, id);
   
            while (!strscan.isEOF())
            {
              strscan.readWord(id, 0);
              dsg->atoms.push(getAtomIdx(id.ptr()));
              strscan.skipSpace();
            }
         }

         TiXmlElement *brackets = elem->FirstChildElement("MBracket"); 
          
         if (brackets != 0)
         {
            const char * brk_type = brackets->Attribute("type");
            if (brk_type != 0)
            {
               if (strncmp(brk_type, "SQUARE", 6) == 0)
               {
                  dsg->brk_style = 0;
               }
               else if (strncmp(brk_type, "ROUND", 5) == 0)
               {
                  dsg->brk_style = 1;
               }
            }

            int point_idx = 0;
            Vec2f *pbrackets;
            TiXmlElement * pPoint;
            for (pPoint = brackets->FirstChildElement();
                 pPoint;
                 pPoint = pPoint->NextSiblingElement())
            {
               if (strncmp(pPoint->Value(), "MPoint", 6) != 0)
                  continue;

               if (point_idx == 0)
                  pbrackets = dsg->brackets.push();
  
               float x,y;
               const char *point_x = pPoint->Attribute("x");
               if (point_x != 0)
               {
                  BufferScanner strscan(point_x);
                  x = strscan.readFloat();
               }
               const char *point_y = pPoint->Attribute("y");
               if (point_y != 0)
               {
                  BufferScanner strscan(point_y);
                  y = strscan.readFloat();
               }
               pbrackets[point_idx].x = x;
               pbrackets[point_idx].y = y;
               point_idx++;
               if (point_idx == 2)
                  point_idx = 0;
            }
         }
   
         const char * fieldname = elem->Attribute("fieldName");
         if (fieldname != 0)
            dsg->name.readString(fieldname, true);
   
         const char * fielddata = elem->Attribute("fieldData");
         if (fieldname != 0)
            dsg->data.readString(fielddata, true);
   
         const char * fieldtype = elem->Attribute("fieldType");
         if (fieldtype != 0)
            dsg->description.readString(fieldtype, true);
   
         const char * disp_x = elem->Attribute("x");
         if (disp_x != 0)
         {
            BufferScanner strscan(disp_x);
            dsg->display_pos.x = strscan.readFloat();
         }
   
         const char * disp_y = elem->Attribute("y");
         if (disp_y != 0)
         {
            BufferScanner strscan(disp_y);
            dsg->display_pos.y = strscan.readFloat();
         }
   
         const char * detached = elem->Attribute("dataDetached");
         dsg->detached = true;
         if (detached != 0)
         {
            if ( (strncmp(detached, "true", 4) == 0) ||
                 (strncmp(detached, "on", 2) == 0) ||
                 (strncmp(detached, "1", 1) == 0) ||
                 (strncmp(detached, "yes", 3) == 0) )
            {
               dsg->detached = true;
            }
           else if ( (strncmp(detached, "false", 5) == 0) ||
                 (strncmp(detached, "off", 3) == 0) ||
                 (strncmp(detached, "0", 1) == 0) ||
                 (strncmp(detached, "no", 2) == 0) )
            {
               dsg->detached = false;
            }
         }
   
         const char * relative = elem->Attribute("placement");
         dsg->relative = false;
         if (relative != 0)
         {
            if (strncmp(relative, "Relative", 8) == 0)
            {
               dsg->relative = true;
            }
         }
   
   
         const char * disp_units = elem->Attribute("unitsDisplayed");
         if (disp_units != 0)
         {
            if ( (strncmp(disp_units, "Unit displayed", 14) == 0) ||
                 (strncmp(disp_units, "yes", 3) == 0) ||
                 (strncmp(disp_units, "on", 2) == 0) ||
                 (strncmp(disp_units, "1", 1) == 0) ||
                 (strncmp(disp_units, "true", 4) == 0) )
            {
               dsg->display_units = true;
            }
         }
   
         dsg->num_chars = 0;
         const char * disp_chars = elem->Attribute("displayedChars");
         if (disp_chars != 0)
         {
            BufferScanner strscan(disp_chars);
            dsg->num_chars = strscan.readInt1();
         }
   
         const char * disp_tag = elem->Attribute("tag");
         if (disp_tag != 0)
         {
            BufferScanner strscan(disp_tag);
            dsg->tag = strscan.readChar();
         }
   
         const char * query_op = elem->Attribute("queryOp");
         if (query_op != 0)
            dsg->queryoper.readString(query_op, true);
   
         const char * query_type = elem->Attribute("queryType");
         if (query_type != 0)
            dsg->querycode.readString(query_type, true);
   
         TiXmlNode * pChild;
         for (pChild = elem->FirstChild();
              pChild != 0;
              pChild = pChild->NextSibling())
         {
            if (strncmp(pChild->Value(), "molecule", 8) != 0)
               continue;
            TiXmlHandle next_mol = pChild;
            if (next_mol.Element() != 0)
               _loadSGroup(next_mol.Element(), mol, atoms_id, idx + 1);
         }
      }
      else if (gen != 0)
      {
         if (sg_parent > 0)
            gen->parent_group = sg_parent;
   
         const char *atom_refs = elem->Attribute("atomRefs");
         if (atom_refs != 0)
         {
            BufferScanner strscan(atom_refs);
            QS_DEF(Array<char>, id);
   
            while (!strscan.isEOF())
            {
              strscan.readWord(id, 0);
              gen->atoms.push(getAtomIdx(id.ptr()));
              strscan.skipSpace();
            }
         }

         TiXmlElement *brackets = elem->FirstChildElement("MBracket"); 
          
         if (brackets != 0)
         {
            const char * brk_type = brackets->Attribute("type");
            if (brk_type != 0)
            {
               if (strncmp(brk_type, "SQUARE", 6) == 0)
               {
                  gen->brk_style = 0;
               }
               else if (strncmp(brk_type, "ROUND", 5) == 0)
               {
                  gen->brk_style = 1;
               }
            }

            int point_idx = 0;
            Vec2f *pbrackets;
            TiXmlElement * pPoint;
            for (pPoint = brackets->FirstChildElement();
                 pPoint;
                 pPoint = pPoint->NextSiblingElement())
            {
               if (strncmp(pPoint->Value(), "MPoint", 6) != 0)
                  continue;

               if (point_idx == 0)
                  pbrackets = gen->brackets.push();
  
               float x,y;
               const char *point_x = pPoint->Attribute("x");
               if (point_x != 0)
               {
                  BufferScanner strscan(point_x);
                  x = strscan.readFloat();
               }
               const char *point_y = pPoint->Attribute("y");
               if (point_y != 0)
               {
                  BufferScanner strscan(point_y);
                  y = strscan.readFloat();
               }
               pbrackets[point_idx].x = x;
               pbrackets[point_idx].y = y;
               point_idx++;
               if (point_idx == 2)
                  point_idx = 0;
            }
         }

         TiXmlNode * pChild;
         for (pChild = elem->FirstChild();
              pChild != 0;
              pChild = pChild->NextSibling())
         {
            if (strncmp(pChild->Value(), "molecule", 8) != 0)
               continue;
            TiXmlHandle next_mol = pChild;
            if (next_mol.Element() != 0)
               _loadSGroup(next_mol.Element(), mol, atoms_id, idx + 1);
         }
      }
      else if (sru != 0)
      {
         if (sg_parent > 0)
            sru->parent_group = sg_parent;
   
         const char *atom_refs = elem->Attribute("atomRefs");
         if (atom_refs != 0)
         {
            BufferScanner strscan(atom_refs);
            QS_DEF(Array<char>, id);
   
            while (!strscan.isEOF())
            {
              strscan.readWord(id, 0);
              sru->atoms.push(getAtomIdx(id.ptr()));
              strscan.skipSpace();
            }
         }

         TiXmlElement *brackets = elem->FirstChildElement("MBracket"); 
          
         if (brackets != 0)
         {
            const char * brk_type = brackets->Attribute("type");
            if (brk_type != 0)
            {
               if (strncmp(brk_type, "SQUARE", 6) == 0)
               {
                  sru->brk_style = 0;
               }
               else if (strncmp(brk_type, "ROUND", 5) == 0)
               {
                  sru->brk_style = 1;
               }
            }

            int point_idx = 0;
            Vec2f *pbrackets;
            TiXmlElement * pPoint;
            for (pPoint = brackets->FirstChildElement();
                 pPoint;
                 pPoint = pPoint->NextSiblingElement())
            {
               if (strncmp(pPoint->Value(), "MPoint", 6) != 0)
                  continue;

               if (point_idx == 0)
                  pbrackets = sru->brackets.push();
  
               float x,y;
               const char *point_x = pPoint->Attribute("x");
               if (point_x != 0)
               {
                  BufferScanner strscan(point_x);
                  x = strscan.readFloat();
               }
               const char *point_y = pPoint->Attribute("y");
               if (point_y != 0)
               {
                  BufferScanner strscan(point_y);
                  y = strscan.readFloat();
               }
               pbrackets[point_idx].x = x;
               pbrackets[point_idx].y = y;
               point_idx++;
               if (point_idx == 2)
                  point_idx = 0;
            }
         }

         const char * title = elem->Attribute("title");
         if (title != 0)
            sru->subscript.readString(title, true);

         const char * connect = elem->Attribute("connect");
         if (connect != 0)
         {
            if (strncmp(connect, "ht", 2) == 0)
            {
               sru->connectivity = SGroup::HEAD_TO_TAIL;
            }
            else if (strncmp(connect, "hh", 2) == 0)
            {
               sru->connectivity = SGroup::HEAD_TO_HEAD;
            }
         }

         TiXmlNode * pChild;
         for (pChild = elem->FirstChild();
              pChild != 0;
              pChild = pChild->NextSibling())
         {
            if (strncmp(pChild->Value(), "molecule", 8) != 0)
               continue;
            TiXmlHandle next_mol = pChild;
            if (next_mol.Element() != 0)
               _loadSGroup(next_mol.Element(), mol, atoms_id, idx + 1);
         }
      }
      else if (mul != 0)
      {
         if (sg_parent > 0)
            mul->parent_group = sg_parent;
   
         const char *atom_refs = elem->Attribute("atomRefs");
         if (atom_refs != 0)
         {
            BufferScanner strscan(atom_refs);
            QS_DEF(Array<char>, id);
   
            while (!strscan.isEOF())
            {
              strscan.readWord(id, 0);
              mul->atoms.push(getAtomIdx(id.ptr()));
              strscan.skipSpace();
            }
         }

         const char *patoms = elem->Attribute("patoms");
         if (patoms != 0)
         {
            BufferScanner strscan(patoms);
            QS_DEF(Array<char>, id);
   
            while (!strscan.isEOF())
            {
              strscan.readWord(id, 0);
              mul->parent_atoms.push(getAtomIdx(id.ptr()));
              strscan.skipSpace();
            }
         }

         TiXmlElement *brackets = elem->FirstChildElement("MBracket"); 
          
         if (brackets != 0)
         {
            const char * brk_type = brackets->Attribute("type");
            if (brk_type != 0)
            {
               if (strncmp(brk_type, "SQUARE", 6) == 0)
               {
                  mul->brk_style = 0;
               }
               else if (strncmp(brk_type, "ROUND", 5) == 0)
               {
                  mul->brk_style = 1;
               }
            }

            int point_idx = 0;
            Vec2f *pbrackets;
            TiXmlElement * pPoint;
            for (pPoint = brackets->FirstChildElement();
                 pPoint;
                 pPoint = pPoint->NextSiblingElement())
            {
               if (strncmp(pPoint->Value(), "MPoint", 6) != 0)
                  continue;

               if (point_idx == 0)
                  pbrackets = mul->brackets.push();
  
               float x,y;
               const char *point_x = pPoint->Attribute("x");
               if (point_x != 0)
               {
                  BufferScanner strscan(point_x);
                  x = strscan.readFloat();
               }
               const char *point_y = pPoint->Attribute("y");
               if (point_y != 0)
               {
                  BufferScanner strscan(point_y);
                  y = strscan.readFloat();
               }
               pbrackets[point_idx].x = x;
               pbrackets[point_idx].y = y;
               point_idx++;
               if (point_idx == 2)
                  point_idx = 0;
            }
         }

         const char * title = elem->Attribute("title");
         if (title != 0)
         {
            BufferScanner strscan(title);
            mul->multiplier = strscan.readInt1();
         }

         TiXmlNode * pChild;
         for (pChild = elem->FirstChild();
              pChild != 0;
              pChild = pChild->NextSibling())
         {
            if (strncmp(pChild->Value(), "molecule", 8) != 0)
               continue;
            TiXmlHandle next_mol = pChild;
            if (next_mol.Element() != 0)
               _loadSGroup(next_mol.Element(), mol, atoms_id, idx + 1);
         }
      }
      else if (sup != 0)
      {
         if (sg_parent > 0)
            sup->parent_group = sg_parent;
   
         const char *atom_refs = elem->Attribute("atomRefs");
         if (atom_refs != 0)
         {
            BufferScanner strscan(atom_refs);
            QS_DEF(Array<char>, id);
   
            while (!strscan.isEOF())
            {
              strscan.readWord(id, 0);
              sup->atoms.push(getAtomIdx(id.ptr()));
              strscan.skipSpace();
            }
         }

         TiXmlElement *brackets = elem->FirstChildElement("MBracket"); 
          
         if (brackets != 0)
         {
            const char * brk_type = brackets->Attribute("type");
            if (brk_type != 0)
            {
               if (strncmp(brk_type, "SQUARE", 6) == 0)
               {
                  sup->brk_style = 0;
               }
               else if (strncmp(brk_type, "ROUND", 5) == 0)
               {
                  sup->brk_style = 1;
               }
            }

            int point_idx = 0;
            Vec2f *pbrackets;
            TiXmlElement * pPoint;
            for (pPoint = brackets->FirstChildElement();
                 pPoint;
                 pPoint = pPoint->NextSiblingElement())
            {
               if (strncmp(pPoint->Value(), "MPoint", 6) != 0)
                  continue;

               if (point_idx == 0)
                  pbrackets = sup->brackets.push();
  
               float x,y;
               const char *point_x = pPoint->Attribute("x");
               if (point_x != 0)
               {
                  BufferScanner strscan(point_x);
                  x = strscan.readFloat();
               }
               const char *point_y = pPoint->Attribute("y");
               if (point_y != 0)
               {
                  BufferScanner strscan(point_y);
                  y = strscan.readFloat();
               }
               pbrackets[point_idx].x = x;
               pbrackets[point_idx].y = y;
               point_idx++;
               if (point_idx == 2)
                  point_idx = 0;
            }
         }
/*
         TiXmlElement *atpoints = elem->FirstChildElement("AttachmentPointArray"); 

         if (atpoints != 0)
         {
            TiXmlElement * aPoint;
            for (aPoint = atpoints->FirstChildElement();
                 aPoint;
                 aPoint = aPoint->NextSiblingElement())
            {
               if (strncmp(aPoint->Value(), "attachmentPoint", 15) != 0)
                  continue;

               int idap = sup->attachment_points.add();
               Superatom::_AttachmentPoint &ap = sup->attachment_points.at(idap);

               const char *a_idx = aPoint->Attribute("atom");
               if (a_idx != 0)
               {
                  ap.aidx = getAtomIdx(a_idx);
               }

               const char *b_idx = aPoint->Attribute("bond");
               if (b_idx != 0)
               {
                  ap.aidx = getAtomIdx(a_idx);
               }

               const char *ap_order = aPoint->Attribute("order");
               if (ap_order != 0)
               {
                  ap.aidx = getAtomIdx(a_idx);
               }

            }
         }
*/

         const char * title = elem->Attribute("title");
         if (title != 0)
            sup->subscript.readString(title, true);

         TiXmlNode * pChild;
         for (pChild = elem->FirstChild();
              pChild != 0;
              pChild = pChild->NextSibling())
         {
            if (strncmp(pChild->Value(), "molecule", 8) != 0)
               continue;
            TiXmlHandle next_mol = pChild;
            if (next_mol.Element() != 0)
               _loadSGroup(next_mol.Element(), mol, atoms_id, idx + 1);
         }
      }
   }
}

void MoleculeCmlLoader::_loadRgroup (TiXmlHandle &handle, Molecule &mol)
{
   MoleculeRGroups *rgroups = &mol.rgroups;

   TiXmlElement *elem = handle.Element();
   if (elem != 0)
   {
      int rg_idx;
      const char *rgroup_id = elem->Attribute("rgroupID");
      if (rgroup_id == 0)
         throw Error("Rgroup without ID");
      BufferScanner strscan(rgroup_id);
      rg_idx = strscan.readInt1();

      RGroup &rgroup = rgroups->getRGroup(rg_idx);

      const char *rlogic_range = elem->Attribute("rlogicRange");
      if (rlogic_range != 0)
         _parseRlogicRange (rlogic_range, rgroup.occurrence);

      const char *rgroup_then = elem->Attribute("thenR");
      if (rgroup_then != 0)
      {
         BufferScanner strscan(rgroup_then);
         rgroup.if_then = strscan.readInt1();
      }

      rgroup.rest_h = 0;
      const char *rgroup_resth = elem->Attribute("restH");
      if (rgroup_resth != 0)
      {
         if ( (strncmp(rgroup_resth, "true", 4) == 0) ||
              (strncmp(rgroup_resth, "on", 2) == 0) ||
              (strncmp(rgroup_resth, "1", 1) == 0) )
         {
            rgroup.rest_h = 1;
         }
      }

      TiXmlNode * pChild;
      for (pChild = handle.FirstChild().ToNode();
           pChild != 0;
           pChild = pChild->NextSibling())
      {
         if (strncmp(pChild->Value(), "molecule", 8) != 0)
            continue;
         TiXmlHandle molecule = pChild;
         if (molecule.Element() != 0)
         {
            AutoPtr<BaseMolecule> fragment(mol.neu());
           _loadMolecule (molecule, fragment.get()->asMolecule());
           rgroup.fragments.add(fragment.release());
         }
      }
   }
}

void MoleculeCmlLoader::_parseRlogicRange (const char *str, Array<int> &ranges)
{
   int beg = -1, end = -1;
   int add_beg = 0, add_end = 0;

   while (*str != 0)
   {
      if (*str == '>')
      {
         end = 0xFFFF;
         add_beg = 1;
      } else if (*str == '<')
      {
         beg = 0;
         add_end = -1;
      } else if (isdigit(*str))
      {
         sscanf(str, "%d", beg == -1 ? &beg : &end);
         while (isdigit(*str))
            str++;
         continue;
      } else if (*str == ',')
      {
         if (end == -1)
            end = beg;
         else
            beg += add_beg, end += add_end;
         ranges.push((beg << 16) | end);
         beg = end = -1;
         add_beg = add_end = 0;
      }
      str++;
   }

   if (beg == -1 && end == -1)
      return;

   if (end == -1)
      end = beg;
   else
      beg += add_beg, end += add_end;
   ranges.push((beg << 16) | end);
}
