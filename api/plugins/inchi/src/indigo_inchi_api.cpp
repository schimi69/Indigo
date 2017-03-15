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

#include "molecule/inchi_wrapper.h"

#include "indigo_internal.h"
#include "indigo_molecule.h"

using namespace indigo;

CEXPORT const char* indigoInchiVersion()
{
   return InchiWrapper::version();
}

//
// Session Inchi instance
//
class IndigoInchiContext : public IndigoPluginContext
{
public:
   InchiWrapper inchi;

   virtual void init()
   {
      inchi.clear();
   }
};

_SessionLocalContainer<IndigoInchiContext> inchi_wrapper_self;

IndigoInchiContext &indigoInchiGetInstance()
{
   IndigoInchiContext &inst = inchi_wrapper_self.getLocalCopy();
   inst.validate();
   return inst;
}

//
// C interface functions
//

CEXPORT int indigoInchiResetOptions(void)
{
   IndigoInchiContext &inchi_context = indigoInchiGetInstance();
   inchi_context.init();
   return 0;
}

CEXPORT int indigoInchiLoadMolecule(const char *inchi_string)
{
   INDIGO_BEGIN
   {
      InchiWrapper &inchi_wrapper = indigoInchiGetInstance().inchi;

      AutoPtr<IndigoMolecule> mol_obj(new IndigoMolecule());

      const char *aux_prefix = "AuxInfo";
      if (strncmp(inchi_string, aux_prefix, strlen(aux_prefix)) == 0)
         inchi_wrapper.loadMoleculeFromAux(inchi_string, mol_obj->mol);
      else
         inchi_wrapper.loadMoleculeFromInchi(inchi_string, mol_obj->mol);
      return self.addObject(mol_obj.release());
   }
   INDIGO_END(-1)
}

CEXPORT const char* indigoInchiGetInchi(int molecule)
{
   INDIGO_BEGIN
   {
      InchiWrapper &inchi_wrapper = indigoInchiGetInstance().inchi;
      IndigoObject &obj = self.getObject(molecule);

      auto &tmp = self.getThreadTmpData();
      inchi_wrapper.saveMoleculeIntoInchi(obj.getMolecule(), tmp.string);
      return tmp.string.ptr();
   }
   INDIGO_END(0)
}

CEXPORT const char* indigoInchiGetInchiKey(const char *inchi_string)
{
   INDIGO_BEGIN
   {
      auto &tmp = self.getThreadTmpData();
      InchiWrapper::InChIKey(inchi_string, tmp.string);
      return tmp.string.ptr();
   }
   INDIGO_END(0)
}

CEXPORT const char* indigoInchiGetWarning()
{
   InchiWrapper &inchi_wrapper = indigoInchiGetInstance().inchi;
   if (inchi_wrapper.warning.size() != 0)
      return inchi_wrapper.warning.ptr();
   return "";
}

CEXPORT const char* indigoInchiGetLog()
{
   InchiWrapper &inchi_wrapper = indigoInchiGetInstance().inchi;
   if (inchi_wrapper.log.size() != 0)
      return inchi_wrapper.log.ptr();
   return "";
}

CEXPORT const char* indigoInchiGetAuxInfo()
{
   InchiWrapper &inchi_wrapper = indigoInchiGetInstance().inchi;
   if (inchi_wrapper.auxInfo.size() != 0)
      return inchi_wrapper.auxInfo.ptr();
   return "";
}

//
// Options
//

void indigoInchiSetInchiOptions (const char *options)
{
   InchiWrapper &inchi_wrapper = indigoInchiGetInstance().inchi;
   inchi_wrapper.setOptions(options);
}

void indigoInchiGetInchiOptions (Array<char>& value)
{
   InchiWrapper &inchi_wrapper = indigoInchiGetInstance().inchi;
   inchi_wrapper.getOptions(value);
}

class _IndigoInchiOptionsHandlersSetter
{
public:
   _IndigoInchiOptionsHandlersSetter ();
};

_IndigoInchiOptionsHandlersSetter::_IndigoInchiOptionsHandlersSetter ()
{
   OptionManager &mgr = indigoGetOptionManager();
   OsLocker locker(mgr.lock);

   mgr.setOptionHandlerString("inchi-options", indigoInchiSetInchiOptions, indigoInchiGetInchiOptions);
}

_IndigoInchiOptionsHandlersSetter _indigo_inchi_options_handlers_setter;
