#ifndef INTERNALS_HPP_INCLUDED
#define INTERNALS_HPP_INCLUDED

#include <map>
#include "common.h"

template <class KType, class AType>
class TreeNode
{
public:
  // must return NULL on lookup failure
  virtual AType* find(KType* key) = 0;
};


template <class KType, class AType>
class ActionNode : public TreeNode<KType,AType>
{
public:
  ActionNode(AType *action) : action(action){}
  AType* find(KType *key) {return action;}

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
public:
  RangeOpNode(AType *outside_action, RangeOperator_t *op, AType *inside_action);

private:
  TreeNode<KType,AType> *inside_range;
  TreeNode<KType,AType> *outside_range;
};


template <class KType, class AType>
class PunctOpNode : public OpNode<KType,AType>
{
public:
  PunctOpNode(AType *default_action, RangeOperator_t op);

  void addPuntAction(KType *key, AType *action){
    others[key]=action;
  }

  AType* find(KType* key){
    typename std::map<KType*,AType*>::iterator i = others.find(key);
    if (i == others.end)
      return dfl_action;
    return i.second;
  }

private:
  // PunctOpNode(s) must be leaves in the tree, so they store directly
  // the AType(s), rather than connecting to ActionNode(s) or TreeNode(s)
  AType *dfl_action;
  std::map<KType*,AType*> others;
};


#endif /* INTERNALS_HPP_INCLUDED */
