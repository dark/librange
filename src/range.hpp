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
  Range(AType* def_action);
  static Range intersect(Range a, Range b, merger_func_t merger, void* extra_info);
  AType* find(KType* key);

private:
};


/* == template implementation follows == */
template <class KType, class AType>
Range<KType,AType>::Range(AType* default_action)
{
  
}

template <class KType, class AType>
Range<KType,AType> Range<KType,AType>::intersect(Range a, Range b, merger_func_t merger, void* extra_info)
{
  AType *t1, *t2;
  AType *t3 = (*merger)(t1,t2,extra_info);

  return a;
}


#endif /* RANGE_HPP_INCLUDED */
