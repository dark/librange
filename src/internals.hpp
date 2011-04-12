#ifndef INTERNALS_HPP_INCLUDED
#define INTERNALS_HPP_INCLUDED

#include "common.h"

template <class KType, class AType>
class TreeNode
{
public:
  // returns NULL on lookup failure
  virtual AType* find(KType* key) = 0;
};


template <class KType, class AType>
class ActionNode : public TreeNode<KType,AType>
{
public:
  ActionNode(AType *action) : action(action){}
  AType* find(KType* key) {return action;}
private:
  AType *action;
};


template <class KType, class AType>
class OpNode : public TreeNode<KType,AType>
{
public:
  OpNode(RangeOperator_t op) : op(op) {}
  RangeOperator_t getOp() {return op;}
private:
  RangeOperator_t op;
};


template <class KType, class AType>
class RangeOpNode : public OpNode<KType,AType>
{
};


template <class KType, class AType>
class PunctOpNode : public OpNode<KType,AType>
{
};


#endif /* INTERNALS_HPP_INCLUDED */
