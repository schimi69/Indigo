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

#include "molecule/molecule_mass.h"

#include "molecule/molecule.h"
#include "molecule/elements.h"

using namespace indigo;

MoleculeMass::MoleculeMass()
{
   relative_atomic_mass_map = NULL;
}

float MoleculeMass::molecularWeight (Molecule &mol)
{
   mol.restoreAromaticHydrogens();

   double molmass = 0;
   int impl_h = 0;
   int elements_count[ELEM_MAX] = {0};

   for (int v = mol.vertexBegin(); 
            v != mol.vertexEnd(); 
            v = mol.vertexNext(v))
   {
      if (mol.isPseudoAtom(v) || mol.isRSite(v) || mol.isTemplateAtom(v))
      {
         continue;
      }

      int number = mol.getAtomNumber(v);
      int isotope = mol.getAtomIsotope(v);
      

      if (isotope == 0)
      {
         float *value = 0;
         if (relative_atomic_mass_map != NULL)
         {
            value = relative_atomic_mass_map->at2(number);
         }

         if (value == 0)
         {
            elements_count[number]++;
         }
         else
         {
            molmass += *value;
         }
      }
      else
      {
         molmass += Element::getRelativeIsotopicMass(number, isotope);
      }

      // Add hydrogens
      impl_h += mol.getImplicitH(v);
      
   } 

   for (int i = ELEM_MIN; i < ELEM_MAX; i++) 
   {
      if (elements_count[i])
      {
         molmass += Element::getStandardAtomicWeight(i) * (double)elements_count[i];
      }
   }

   molmass += Element::getStandardAtomicWeight(ELEM_H) * impl_h;

   return (float)molmass;
}

static int _isotopesCmp (int i1, int i2, void *context)
{
   int element = *(int *)context;
   float c1, c2;
   Element::getIsotopicComposition(element, i1, c1);
   Element::getIsotopicComposition(element, i2, c2);
   if (c1 < c2)
      return 1;
   else if (c1 > c2)
      return -1;
   return 0;
}

float MoleculeMass::mostAbundantMass (Molecule &mol)
{
   mol.restoreAromaticHydrogens();

   double molmass = 0;

   // Count elements without explicit isotope marks
   int elements_counts[ELEM_MAX] = {0};

   for (int v = mol.vertexBegin(); 
            v != mol.vertexEnd(); 
            v = mol.vertexNext(v))
   {
      if (mol.isPseudoAtom(v) || mol.isTemplateAtom(v))
         continue;

      int number = mol.getAtomNumber(v);
      int isotope = mol.getAtomIsotope(v);
      int impl_h = mol.getImplicitH(v);

      if (isotope == 0)
         elements_counts[number]++;
      else
         molmass += Element::getRelativeIsotopicMass(number, isotope);

      // Add hydrogens
      elements_counts[ELEM_H] += impl_h;
   } 

   QS_DEF(Array<int>, isotopes);

   // Compute mass of the most abunant composition
   for (int i = ELEM_MIN; i < ELEM_MAX; i++)
   {
      int count = elements_counts[i];
      if (count == 0)
         continue;

      int count_left = count;
      int min_iso, max_iso;

      // Sort by isotope abundance
      isotopes.clear();
      Element::getMinMaxIsotopeIndex(i, min_iso, max_iso);
      for (int j = min_iso; j <= max_iso; j++)
      {
         float composition;
         if (!Element::getIsotopicComposition(i, j, composition))
            continue;
         if (composition > 0)
            isotopes.push(j);
      }
      isotopes.qsort(_isotopesCmp, (void *)&i);

      for (int k = 0; k < isotopes.size(); k++) 
      {
         int j = isotopes[k];

         float composition;
         if (!Element::getIsotopicComposition(i, j, composition))
            continue;

         int such_isotope_count = (int)(composition * count / 100 + 0.5f);

         molmass += Element::getRelativeIsotopicMass(i, j) * such_isotope_count;
         count_left -= such_isotope_count;
         if (count_left == 0)
            break;
      }

      if (count_left != 0)
      {
         // Corrections in case of rounding errors
         int default_iso = Element::getMostAbundantIsotope(i);
         molmass += Element::getRelativeIsotopicMass(i, default_iso) * count_left;
      }
   }

   return (float)molmass;
}

float MoleculeMass::monoisotopicMass (Molecule &mol)
{
   mol.restoreAromaticHydrogens();

   double molmass = 0;

   for (int v = mol.vertexBegin(); 
            v != mol.vertexEnd(); 
            v = mol.vertexNext(v))
   {
      if (mol.isPseudoAtom(v) || mol.isTemplateAtom(v))
         continue;

      int number = mol.getAtomNumber(v);
      int isotope = mol.getAtomIsotope(v);
      int impl_h = mol.getImplicitH(v);

      if (isotope == 0)
         isotope = Element::getMostAbundantIsotope(number);

      molmass += Element::getRelativeIsotopicMass(number, isotope);

      // Add hydrogens
      molmass += Element::getRelativeIsotopicMass(ELEM_H, 1) * impl_h;
   } 

   return (float)molmass;
}

int MoleculeMass::nominalMass (Molecule &mol)
{
   mol.restoreAromaticHydrogens();

   int molmass = 0;

   for (int v = mol.vertexBegin(); 
            v != mol.vertexEnd(); 
            v = mol.vertexNext(v))
   {
      if (mol.isPseudoAtom(v) || mol.isTemplateAtom(v))
         continue;  

      int number = mol.getAtomNumber(v);
      int isotope = mol.getAtomIsotope(v);
      int impl_h = mol.getImplicitH(v);

      if (isotope == 0)
         molmass += Element::getDefaultIsotope(number);
      else
         molmass += isotope;

      // Add hydrogens
      molmass += impl_h;
   } 

   return molmass;
}

int MoleculeMass::_cmp (_ElemCounter &ec1, _ElemCounter &ec2, void *context)
{
    if (ec1.weight == 0)
        return 1;
    if (ec2.weight == 0)
        return -1;
    
    if (ec2.elem == ELEM_H) // move hydrogen to the end
        return -1;
    if (ec1.elem == ELEM_H)
        return 1;
    
    return ec1.elem - ec2.elem;
}


void MoleculeMass::massComposition (Molecule &mol, Array<char> &str)
{
    Array<float> relativeMass;
    int impl_h = 0;
    relativeMass.clear_resize(ELEM_MAX);
    relativeMass.zerofill();
    
    mol.restoreAromaticHydrogens();
    
    for (int v = mol.vertexBegin();
         v != mol.vertexEnd();
         v = mol.vertexNext(v))
    {
        if (mol.isPseudoAtom(v) || mol.isTemplateAtom(v))
        {
            continue;
        }
        
        int number = mol.getAtomNumber(v);
        int isotope = mol.getAtomIsotope(v);
        impl_h += mol.getImplicitH(v);
        
        if (isotope)
        {
            relativeMass[number] += Element::getRelativeIsotopicMass(number, isotope);
        }
        else
        {
            float *value = 0;
            if (relative_atomic_mass_map != NULL)
            {
                value = relative_atomic_mass_map->at2(number);
            }
            
            if (value)
            {
                relativeMass[number] += *value;
            }
            else
            {
                relativeMass[number] += Element::getStandardAtomicWeight(number);
            }
        }
    }
    
    relativeMass[ELEM_H] += Element::getStandardAtomicWeight(ELEM_H) * impl_h;
    
    float totalWeight = molecularWeight(mol);
    
    QS_DEF(Array<_ElemCounter>, counters);
    int i;
    
    counters.clear();
    
    for (i = ELEM_MIN; i < ELEM_MAX; i++)
    {
        _ElemCounter &ec = counters.push();
        
        ec.elem = i;
        ec.weight = (relativeMass[i] / totalWeight) * 100;
    }
    
    counters.qsort(_cmp, 0);
    
    ArrayOutput output(str);
    
    bool first_written = false;
    
    for (i = 0; i < counters.size(); i++)
    {
        if (counters[i].weight == 0)
            break;
        
        if (first_written)
            output.printf(" ");
        
        output.printf(Element::toString(counters[i].elem));
        output.printf(" %.2f", counters[i].weight);
        first_written = true;
    }
    output.writeChar(0);
}
