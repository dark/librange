/*
 librange
 Copyright (C) 2011 Marco Leogrande
 
 This file is part of librange.
 
 librange is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 
 librange is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RANGE_HPP_INCLUDED
#define RANGE_HPP_INCLUDED

#include <map>
#include "common.h"
#include "internals.hpp"

/* == important declarations == */

/* KType specifies the type of the data associated with the range keys,
 * AType specifies the type of the action to which each range is associated
 */
template <class KType, class AType>
class Range
{
  typedef AType*(*merger_func_t)(const AType*, const AType*, void*);
  typedef void(*range_callback_func_t)(RangeOperator_t, KType*, void*);
  typedef void(*punt_callback_func_t)(RangeOperator_t, std::map<KType*,AType*>, void*);
  typedef void(*action_callback_func_t)(AType*, void*);

public:
  Range(AType *dfl_action);
  void addRange(RangeOperator_t op, KType *key, AType *action);
  AType* find(KType *key);
  static Range intersect(Range a, Range b, merger_func_t merger, void *extra_info);
  void traverse(range_callback_func_t range_callback, punt_callback_func_t punt_callback, action_callback_func_t action_callback, void *extra_info);

private:
  AType *default_action;
  OpNode<KType,AType> *tree;
};


/* == template implementation follows == */
template <class KType, class AType>
Range<KType,AType>::Range(AType *dfl_action)
  : default_action(dfl_action), tree(NULL)
{
}

template <class KType, class AType>
void Range<KType,AType>::addRange
(RangeOperator_t op, KType *key, AType *action)
{
  if (tree == NULL)
    tree = OpNode<KType,AType>::buildOpNode(default_action, op, key, action);
  else
    tree->addRange(op, key, action);
}

/* returns the action associated with the provided key */
template <class KType, class AType>
AType* Range<KType,AType>::find(KType *key){
  if (tree == NULL)
    return default_action;

  AType *action = tree->find(key);
  if (action == NULL)
    return default_action;
  return action;
}

template <class KType, class AType>
Range<KType,AType> Range<KType,AType>::intersect(Range a, Range b, merger_func_t merger, void* extra_info)
{
  AType *new_dfl = (*merger)(a.default_action, b.default_action, extra_info);
  Range result(new_dfl);

  if(a.tree != NULL && b.tree != NULL) {
    TreeNode<KType,AType> *tmp = TreeMerger<KType,AType>::merge(a.tree, b.tree, merger, extra_info);
    // the following cast is legal, because by construction only an OpNode can be the root of the tree
    result.tree = dynamic_cast<OpNode<KType,AType>*>(tmp);
    if(!result.tree) abort(); // something broke
  } else {
    // otherwise take the not-NULL tree. If both are NULL, set it to NULL (implicit in the assignment below)
    result.tree = (a.tree != NULL? a.tree : b.tree);
  }

  return result;
}

template <class KType, class AType>
void Range<KType,AType>::traverse
(range_callback_func_t range_callback,
 punt_callback_func_t punt_callback,
 action_callback_func_t action_callback,
 void* extra_info)
{
  if (tree)
    tree->traverse(range_callback, punt_callback, action_callback, extra_info);
  else if (action_callback)
    (*action_callback)(default_action, extra_info);
}

#endif /* RANGE_HPP_INCLUDED */