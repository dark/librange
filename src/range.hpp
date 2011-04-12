#ifndef RANGE_HPP_INCLUDED
#define RANGE_HPP_INCLUDED

#include "common.h"
#include "internals.hpp"

/* == important declarations == */

/* KType specifies the type of the data associated with the range keys,
 * AType specifies the type of the action to which each range is associated
 */
template <class KType, class AType>
class Range
{
  typedef AType*(*merger_func_t)(AType*, AType*, void*);
  
public:
  Range(AType *dfl_action);
  void addRange(RangeOperator_t op, KType *key, AType *action);
  AType* find(KType *key);
  static Range intersect(Range a, Range b, merger_func_t merger, void *extra_info);

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
  #warning this code is just a dummy placeholder
  AType *t1, *t2;
  AType *t3 = (*merger)(t1,t2,extra_info);

  return a;
}

#endif /* RANGE_HPP_INCLUDED */
