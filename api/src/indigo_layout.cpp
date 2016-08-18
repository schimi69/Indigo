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

#include "indigo_internal.h"
#include "base_cpp/cancellation_handler.h"
#include "layout/reaction_layout.h"
#include "layout/molecule_layout.h"
#include "reaction/base_reaction.h"
#include "indigo_molecule.h"
#include "indigo_reaction.h"
#include "layout/molecule_cleaner_2d.h"
#include <vector>

CEXPORT int indigoLayout (int object)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(object);
      int i;

      if (IndigoBaseMolecule::is(obj) || obj.type == IndigoObject::SUBMOLECULE) {
         BaseMolecule &mol = obj.getBaseMolecule();
         MoleculeLayout ml(mol, self.smart_layout);
         ml.max_iterations = self.layout_max_iterations;
         ml.bond_length = 1.6f;

         TimeoutCancellationHandler cancellation(self.cancellation_timeout);
         ml.setCancellationHandler(&cancellation);

         Filter f;
         if (obj.type == IndigoObject::SUBMOLECULE)
         {
            IndigoSubmolecule &submol = (IndigoSubmolecule &)obj;
            f.initNone(mol.vertexEnd());
            for (int i = 0; i < submol.vertices.size(); i++)
               f.unhide(submol.vertices[i]);
            ml.filter = &f;
         }

         ml.make();
      
         if (obj.type != IndigoObject::SUBMOLECULE)
         {
            // Not for submolecule yet
            mol.clearBondDirections();
            mol.stereocenters.markBonds();
            mol.allene_stereo.markBonds();
         }

         if (obj.type != IndigoObject::SUBMOLECULE)
         {
            for (i = 1; i <= mol.rgroups.getRGroupCount(); i++)
            {
               RGroup &rgp = mol.rgroups.getRGroup(i);

               for (int j = rgp.fragments.begin(); j != rgp.fragments.end();
                        j = rgp.fragments.next(j))
               {
                  rgp.fragments[j]->clearBondDirections();
                  rgp.fragments[j]->stereocenters.markBonds();
                  rgp.fragments[j]->allene_stereo.markBonds();
               }
            }
         }
      } else if (IndigoBaseReaction::is(obj)) {
         BaseReaction &rxn = obj.getBaseReaction();
         ReactionLayout rl(rxn, self.smart_layout);
         rl.max_iterations = self.layout_max_iterations;
         rl.bond_length = 1.6f;
         rl.horizontal_interval_factor = self.layout_horintervalfactor;

         rl.make();
         rxn.markStereocenterBonds();
      } else {
         throw IndigoError("The object provided is neither a molecule, nor a reaction");
      }
      return 0;
   }
   INDIGO_END(-1)
}

CEXPORT int indigoClean2d(int object)
{
    INDIGO_BEGIN
    {
        IndigoObject &obj = self.getObject(object);

        if (IndigoBaseMolecule::is(obj) || obj.type == IndigoObject::SUBMOLECULE) {
            BaseMolecule &mol = obj.getBaseMolecule();
            MoleculeCleaner2d cleaner2d1(mol, false);
            cleaner2d1.clean(false);
            MoleculeCleaner2d cleaner2d2(mol, true);
            cleaner2d2.clean(true);

        }
        return 0;
    }
    INDIGO_END(-1)
}
