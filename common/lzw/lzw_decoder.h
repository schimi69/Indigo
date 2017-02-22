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

#ifndef __lzw_decoder_h__
#define __lzw_decoder_h__

#include "base_cpp/bitinworker.h"
#include "base_cpp/scanner.h"
#include "lzw/lzw_dictionary.h"

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
#endif

namespace indigo {

class DLLEXPORT LzwDecoder
{
public:
   DECL_ERROR;

   LzwDecoder( LzwDict &NewDict, Scanner &NewIn );

   bool isEOF( void );

   int get( void );

private:

   LzwDict &_dict;

   BitInWorker _bitin;

   CP_DECL;
   TL_CP_DECL(Array<byte>, _symbolsBuf);

   // no implicit copy
   LzwDecoder( const LzwDecoder & );

};

class LzwScanner : public Scanner
{
public:
   LzwScanner (LzwDecoder &decoder);

   virtual void read (int length, void *res);
   virtual void skip (int n);
   virtual bool isEOF ();
   virtual int  lookNext ();
   virtual void seek (long long pos, int from);
   virtual long long length ();
   virtual long long tell ();

   virtual byte readByte ();

   DECL_ERROR;
private:
   LzwDecoder &_decoder;
};

}

#ifdef _WIN32
#pragma warning(pop)
#endif

#endif
