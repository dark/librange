#ifndef INTERNALS_HPP_INCLUDED
#define INTERNALS_HPP_INCLUDED

#include <map>
#include <stdlib.h>
#include "common.h"

enum Node_t
  {
    ACTION = 0,
    RANGE,
    PUNCTUAL
  };

template <class KType, class AType>
class TreeMerger; // fwd decl

template <class KType, class AType>
class TreeNode
{
  friend class TreeMerger<KType, AType>;

  typedef void(*range_callback_func_t)(RangeOperator_t, KType*, void*);
  typedef void(*punt_callback_func_t)(RangeOperator_t, std::map<KType*,AType*>, void*);
  typedef void(*action_callback_func_t)(AType*, void*);

public:
  virtual Node_t getType() = 0;
  // must return NULL on lookup failure
  virtual AType* find(KType* key) = 0;
  virtual void traverse(range_callback_func_t range_callback, punt_callback_func_t punt_callback, action_callback_func_t action_callback, void *extra_info) = 0;
};


template <class KType, class AType>
class ActionNode : public TreeNode<KType,AType>
{
  typedef void(*range_callback_func_t)(RangeOperator_t, KType*, void*);
  typedef void(*punt_callback_func_t)(RangeOperator_t, std::map<KType*,AType*>, void*);
  typedef void(*action_callback_func_t)(AType*, void*);

public:
  ActionNode(AType *action) : action(action){}
  Node_t getType() { return ACTION; }
  AType* find(KType *key) {return action;}
  void traverse(range_callback_func_t range_callback, punt_callback_func_t punt_callback, action_callback_func_t action_callback, void *extra_info)
  { if (action_callback) (*action_callback)(action, extra_info); }

private:
  AType *action;
};


template <class KType, class AType>
class OpNode : public TreeNode<KType,AType>
{
public:
  static OpNode* buildOpNode(AType *dfl_action, RangeOperator_t op, KType *key, AType *cond_action);
  virtual void addRange(RangeOperator_t op, KType *key, AType *cond_action) = 0;
  RangeOperator_t getOp() {return op;}

protected:
  TreeNode<KType,AType> *dfl_node;
  RangeOperator_t op;
};


template <class KType, class AType>
class RangeOpNode : public OpNode<KType,AType>
{
  typedef void(*range_callback_func_t)(RangeOperator_t, KType*, void*);
  typedef void(*punt_callback_func_t)(RangeOperator_t, std::map<KType*,AType*>, void*);
  typedef void(*action_callback_func_t)(AType*, void*);

public:
  RangeOpNode(AType *outside_action)
    : range_separator(NULL), range_node(NULL)
  {
    this->dfl_node = new ActionNode<KType,AType>(outside_action);
  }

  Node_t getType() { return RANGE; }

  void addRange(RangeOperator_t op, KType *key, AType *cond_action)
  {
    if (op == EQUAL || op == INVALID)
      abort();

    if (range_separator != NULL || range_node != NULL)
      return; // ignore the tentative

    this->op = op;
    range_separator = key;
    range_node = new ActionNode<KType,AType>(cond_action);
  }

  AType* find(KType* key){
    bool res;
    switch(this->op){
    case LESS_THAN:
      res = (*key) < (*range_separator);
      break;
    case LESS_EQUAL_THAN:
      res = (*key) <= (*range_separator);
      break;
    case GREAT_THAN:
      res = (*key) > (*range_separator);
      break;
    case GREAT_EQUAL_THAN:
      res = (*key) >= (*range_separator);
      break;
    case EQUAL:
    case INVALID:
      abort();      
    }

    if(res)
      return range_node->find(key);

    return this->dfl_node->find(key);
  }

  void traverse(range_callback_func_t range_callback, punt_callback_func_t punt_callback, action_callback_func_t action_callback, void *extra_info)
  {
    if (range_callback)
      (*range_callback)(this->op, range_separator,  extra_info);

    this->range_node->traverse(range_callback, punt_callback, action_callback, extra_info);
    this->dfl_node->traverse(range_callback, punt_callback, action_callback, extra_info);
  }

private:
  KType *range_separator;
  TreeNode<KType,AType> *range_node;

  RangeOpNode(ActionNode<KType,AType> *dfl_node)
    : range_separator(NULL), range_node(NULL)
  {
    this->dfl_node = dfl_node; 
  }
};


template <class KType, class AType>
class PunctOpNode : public OpNode<KType,AType>
{
  typedef void(*range_callback_func_t)(RangeOperator_t, KType*, void*);
  typedef void(*punt_callback_func_t)(RangeOperator_t, std::map<KType*,AType*>, void*);
  typedef void(*action_callback_func_t)(AType*, void*);

public:
  PunctOpNode(AType *default_action)
  {
    this->dfl_node = new ActionNode<KType,AType>(default_action);
  }

  Node_t getType() { return PUNCTUAL; }

  void addRange(RangeOperator_t op, KType *key, AType *cond_action)
  {
    if (op != EQUAL)
      abort();

    addPuntAction(key, cond_action);
  }

  AType* find(KType* key){
    typename std::map<KType*,AType*>::iterator i = others.find(key);
    if (i == others.end())
      return this->dfl_node->find(key);
    return i->second;
  }

  void traverse(range_callback_func_t range_callback, punt_callback_func_t punt_callback, action_callback_func_t action_callback, void *extra_info)
  {
    if (punt_callback){
      (*punt_callback)(this->op, others, extra_info);
    }

    this->dfl_node->traverse(range_callback, punt_callback, action_callback, extra_info);
  }

private:
  // PunctOpNode(s) must be leaves in the tree, so they store directly
  // the AType(s), rather than connecting to ActionNode(s) or TreeNode(s)
  std::map<KType*,AType*> others;

  void addPuntAction(KType *key, AType *action){
    others[key]=action;
  }
};

template <class KType, class AType>
class TreeMerger
{
  typedef AType*(*merger_func_t)(AType*, AType*, void*);

public:
  static TreeNode<KType, AType>* merge(TreeNode<KType, AType> *a, TreeNode<KType, AType> *b,
                                       merger_func_t merger, void *extra_info)
  {
    Node_t a_type = a->getType();
    Node_t b_type = b->getType();

    // handle immediately the easiest cases
    if (a_type == ACTION && b_type == ACTION) {
      AType m = (*merger)(a->action, b->action, extra_info);
      return new ActionNode<KType, AType>(m);
    }

    // I prefer to have more 'complex' types in 'a' rather than in 'b',
    // so swap them if necessary
    if ( (b_type == RANGE && a_type != RANGE) ||
         (b_type == PUNCTUAL && a_type == ACTION) )
      return merge(b, a, merger, extra_info);

    // handle remaining cases
    if (a_type == RANGE) {
      const RangeOpNode<KType, AType> *a_promoted = dynamic_cast<RangeOpNode<KType, AType>*>(a);
      if (!a) abort(); // something broke

      const RangeOperator_t a_op = a_promoted->getOp();
      const KType *a_separator = a_promoted->range_separator;
      RangeOpNode<KType, AType> *result;
      switch(b_type){
      case RANGE:
        break;

      case PUNCTUAL:
        break;

      case ACTION:
        const TreeNode<KType, AType> *new_dfl_node = merge(a->dfl_node, b, merger, extra_info);
        result = new RangeOpNode<KType, AType>(new_dfl_node);
        result->op = a_promoted->op;
        result->range_separator = a_promoted->range_separator;
        result->range_node =  merge(a->range_node, b, merger, extra_info);
        break;
      }

      return result;
    } else if (a_type == PUNCTUAL) {
    } else {
      abort(); // something broke in the comparisons above
    }
  }
};


/* implementations that needed fwd declarations */
template <class KType, class AType>
OpNode<KType,AType>* OpNode<KType,AType>::buildOpNode
(AType *dfl_action, RangeOperator_t op, KType *key, AType *cond_action)
{
  OpNode<KType,AType> *node;
  switch(op){
  case LESS_THAN:
  case LESS_EQUAL_THAN:
  case GREAT_THAN:
  case GREAT_EQUAL_THAN:
    node = new RangeOpNode<KType,AType>(dfl_action);
    break;
  case EQUAL:
    node = new PunctOpNode<KType,AType>(dfl_action);
    break;
  case INVALID:
    abort();
  }

  node->addRange(op, key, cond_action);
  return node;
}

#endif /* INTERNALS_HPP_INCLUDED */
