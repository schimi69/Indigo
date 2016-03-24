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

#ifndef __tree_h__
#define __tree_h__

#include "base_cpp/exception.h"
#include "base_cpp/obj_array.h"

namespace indigo {

DECL_EXCEPTION(TreeError);

template <typename T> class DataTree
{
public:
   DECL_TPL_ERROR(TreeError);

   explicit DataTree ()
   {
      label = -1;
   }

   ~DataTree ()
   {
   }

   DataTree<T> & push ()
   {
      return push(-1);
   }

   DataTree<T> & push (int label)
   {
      DataTree<T> & child = _children.push();
      child.label = label;
      return child;
   }

   DataTree<T> & push (const T data)
   {
      DataTree<T> & child = push();
      child.data = data;
      return child;
   }

   DataTree<T> & push (int label, const T data)
   {
      DataTree<T> & child = push(label);
      child.data = data;
      return child;
   }

   //There is special value for root = -1
   DataTree<T> & provide (int label)
   {
      if (label == -1) {
         return *this;
      }
      DataTree<T> * ptr = _find_or_remove(label, false);
      if (ptr != nullptr) {
         return *ptr;
      }
      return push(label);
   }

   //There is special value for root = -1
   DataTree<T> & insert (int label, int parent)
   {
      DataTree<T> & ptree = provide(parent);
      return ptree.push(label);
   }

   bool remove (int label)
   {
      return _find_or_remove(label, true) != nullptr;
   }

   bool find (int label)
   {
      return _find_or_remove(label, false) != nullptr;
   }

   ObjArray<DataTree<T>> & children() {
      return _children;
   }

   int label;
   T   data;

protected:
   ObjArray<DataTree<T>> _children;

private:
   DataTree(const DataTree<T> &); // no implicit copy
   DataTree<T>& operator= (const DataTree<T>& other); // no copy constructor

   DataTree<T> * _find_or_remove (int label, bool remove)
   {
      for (int i = 0; i < _children.size(); i++) {
         DataTree<T> & child = _children[i];
         if (child.label == label) {
            if (remove) {
               _children.remove(i);
            }
            return &child;
         } else {
            DataTree<T> * result = child._find_or_remove(label, remove);
            if (result != nullptr) {
               return result;
            }
         }
      }
      return nullptr;
   }
};

struct Empty {};

typedef DataTree<Empty> Tree;

}

#endif