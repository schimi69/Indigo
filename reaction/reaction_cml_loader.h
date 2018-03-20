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

#ifndef __reaction_cml_loader__
#define __reaction_cml_loader__

#include "base_cpp/exception.h"
#include "molecule/molecule_stereocenter_options.h"

namespace indigo
{

class Scanner;
class Reaction;

class ReactionCmlLoader
{
public:

   DECL_ERROR;

   ReactionCmlLoader (Scanner &scanner);
   ~ReactionCmlLoader ();

   void loadReaction (Reaction &rxn);

   StereocentersOptions stereochemistry_options;
   bool ignore_bad_valence;

protected:
   Scanner &_scanner;

private:
   ReactionCmlLoader (const ReactionCmlLoader &); // no implicit copy
};

};

#endif
