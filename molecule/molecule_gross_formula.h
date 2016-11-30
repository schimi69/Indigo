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

#ifndef __molecule_gross_formula__
#define __molecule_gross_formula__

#include <utility>

#include "base_cpp/array.h"
#include "base_cpp/output.h"
#include "base_cpp/scanner.h"

namespace indigo {

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
#endif

class BaseMolecule;

class DLLEXPORT MoleculeGrossFormula
{
public:
   static void collect (BaseMolecule &molecule, std::pair<ObjArray<Array<char> >, ObjArray<Array<int> > > &gross);
   static void toString (const std::pair<ObjArray<Array<char> >, ObjArray<Array<int> > > &gross, Array<char> &str);
   static void toString_Hill (const std::pair<ObjArray<Array<char> >, ObjArray<Array<int> > > &gross, Array<char> &str);
   static void fromString (const char *str, Array<int> &gross);
   static void fromString (Scanner &scanner, Array<int> &gross);

   static bool leq  (const Array<int> &gross1, const Array<int> &gross2);
   static bool geq  (const Array<int> &gross1, const Array<int> &gross2);
   static bool equal (const Array<int> &gross1, const Array<int> &gross2);

protected:
   struct _ElemCounter
   {
      int elem;
      int counter;
   };

   static void _toString (const Array<int> &gross, ArrayOutput &output,
                          int (*cmp)(_ElemCounter &, _ElemCounter &, void *));
   static int _cmp      (_ElemCounter &ec1, _ElemCounter &ec2, void *context);
   static int _cmp_hill (_ElemCounter &ec1, _ElemCounter &ec2, void *context);
   static int _cmp_hill_no_carbon (_ElemCounter &ec1, _ElemCounter &ec2, void *context);
};

}

#endif

