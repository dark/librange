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
  inline RangeOperator_t getOp() {return op;}
  inline RangeOperator_t getNormalizedOp() {
    if (op == LESS_THAN || op == GREAT_EQUAL_THAN)
      return LESS_THAN;
    if (op == LESS_EQUAL_THAN || op == GREAT_THAN)
      return LESS_EQUAL_THAN;
    return op;
  }

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

  inline TreeNode<KType,AType> *left_interval(){
    if (this->op == EQUAL || this->op == INVALID)
      abort(); // something broke
    if (this->op == LESS_THAN || this->op == LESS_EQUAL_THAN)
      return range_node;
    return this->dfl_node;
  }

  inline TreeNode<KType,AType> *right_interval(){
    if (this->op == EQUAL || this->op == INVALID)
      abort(); // something broke
    if (this->op == LESS_THAN || this->op == LESS_EQUAL_THAN)
      return this->dfl_node;
    return this->range_node;
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

  PunctOpNode(ActionNode<KType,AType> *dfl_node)
  {
    this->dfl_node = dfl_node; 
  }

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
      AType *m = (*merger)(a->action, b->action, extra_info);
      return new ActionNode<KType, AType>(m);
    }

    // I prefer to have more 'complex' types in 'a' rather than in 'b',
    // so swap them if necessary
    if ( (b_type == RANGE && a_type != RANGE) ||
         (b_type == PUNCTUAL && a_type == ACTION) )
      return merge(b, a, merger, extra_info);

    // handle remaining cases
    if (a_type == RANGE) {
      // the left node is a RangeOpNode
      const RangeOpNode<KType, AType> *a_prom_range = dynamic_cast<RangeOpNode<KType, AType>*>(a);
      if (!a_prom_range) abort(); // something broke

      RangeOpNode<KType, AType> *result_range;
      switch(b_type){
      case RANGE:
        // the right node is a RangeOpNode
        const RangeOpNode<KType, AType> *b_prom_range = dynamic_cast<RangeOpNode<KType, AType>*>(b);
        if (!b_prom_range) abort(); // something broke
        result_range = merge_range_range(a_prom_range, b_prom_range, merger, extra_info);
        break;

      case PUNCTUAL:
        // the right node is a PunctOpNode
        const PunctOpNode<KType, AType> *b_prom_punct = dynamic_cast<PunctOpNode<KType, AType>*>(b);
        if (!b_prom_punct) abort(); // something broke
        result_range = merge_range_punct(a_prom_range, b_prom_punct, merger, extra_info);
        break;

      case ACTION:
        // the right node is a ActionNode
        const TreeNode<KType, AType> *new_dfl_node = merge(a->dfl_node, b, merger, extra_info);
        result_range = new RangeOpNode<KType, AType>(new_dfl_node);
        result_range->op = a_prom_range->op;
        result_range->range_separator = a_prom_range->range_separator;
        result_range->range_node =  merge(a->range_node, b, merger, extra_info);
        break;
      }

      return result_range;
    } else if (a_type == PUNCTUAL) {
      // the left node is a PunctOpNode
      const PunctOpNode<KType, AType> *a_prom_punct = dynamic_cast<PunctOpNode<KType, AType>*>(a);
      if (!a_prom_punct) abort(); // something broke

      PunctOpNode<KType, AType> *result_punct;
      switch(b_type){
      case PUNCTUAL:
        // the right node is a PunctOpNode
        const PunctOpNode<KType, AType> *b_prom_punct = dynamic_cast<PunctOpNode<KType, AType>*>(b);
        if (!b_prom_punct) abort(); // something broke
        result_punct = merge_punct_punct(a_prom_punct, b_prom_punct, merger, extra_info);
        break;

      case ACTION:
        // the right node is a ActionNode
        const TreeNode<KType, AType> *new_dfl_node = merge(a->dfl_node, b, merger, extra_info);
        const AType *b_action = dynamic_cast<ActionNode<KType, AType>*>(b)->action;
        result_punct = new PunctOpNode<KType, AType>(new_dfl_node);
        result_punct->op = EQUAL;
        for(typename std::map<KType*,AType*>::iterator i = a_prom_punct->others.begin();
            i != a_prom_punct->others.end();
            ++i) {
          result_punct->others[i->first]=(*merger)(i->second, b_action, extra_info);
        }
        break;
      }

      return result_punct;
    } else {
      abort(); // something broke in the comparisons above
    }
  }


private:
  static TreeNode<KType, AType>* merge_range_range(RangeOpNode<KType, AType> *a, RangeOpNode<KType, AType> *b,
                                                   merger_func_t merger, void *extra_info)
  {
    const RangeOperator_t a_op = a->getOp();
    const KType *a_separator = a->range_separator;
    const RangeOperator_t b_op = b->getOp();
    const KType *b_separator = b->range_separator;

    // except when a_separator == b_separator, the intersection
    // between two ranges gives three intervals and two separators
    TreeNode<KType, AType> *int_1=NULL, *int_2=NULL, *int_3=NULL;
    RangeOperator_t sep_1=INVALID, sep_2=INVALID;
    KType *sep_1_val=NULL, *sep_2_val=NULL;
    if(a_separator == b_separator) {
      int_1 = merge(a->left_interval(),
                    b->left_interval(),
                    merger, extra_info);
      int_3 = merge(a->right_interval(),
                    b->right_interval(),
                    merger, extra_info);
      sep_1 = a->getNormalizedOp();
      sep_2 = b->getNormalizedOp();
      sep_1_val = sep_2_val = a_separator;
      if(a->getNormalizedOp() != b->getNormalizedOp()) {
        // there is a small "gap" between the intervals (as in '<x' and '>x')
        // or they are overlapped ('<=x' and '>=x')
        // handle both cases here
        int_2 = merge((sep_1 == LESS_THAN? a->right_interval() :  a->left_interval),
                      (sep_1 == LESS_THAN? a->left_interval() :  a->right_interval),
                      merger, extra_info);
        sep_1 = LESS_THAN;
        sep_2 = LESS_EQUAL_THAN;
      } 
    } else if(a_separator < b_separator) {
      int_1 = merge(a->left_interval(),
                    b->left_interval(),
                    merger, extra_info);
      int_2 = merge(a->right_interval(),
                    b->left_interval(),
                    merger, extra_info);
      int_3 = merge(a->right_interval(),
                    b->right_interval(),
                    merger, extra_info);
      sep_1 = a->getNormalizedOp();
      sep_2 = b->getNormalizedOp();
      sep_1_val = a_separator;
      sep_2_val = b_separator;
    } else {
      // a_separator > b_separator
      int_1 = merge(a->left_interval(),
                    b->left_interval(),
                    merger, extra_info);
      int_2 = merge(a->left_interval(),
                    b->right_interval(),
                    merger, extra_info);
      int_3 = merge(a->right_interval(),
                    b->right_interval(),
                    merger, extra_info);
      sep_1 = b->getNormalizedOp();
      sep_2 = a->getNormalizedOp();
      sep_1_val = b_separator;
      sep_2_val = a_separator;
    }
        
    // if int_2 == NULL, sep_2 will be ignored
    if (int_2 != NULL) {
      RangeOpNode<KType,AType> *tmp = new RangeOpNode<KType,AType>(int_3);
      tmp->op = sep_2;
      tmp->range_separator = sep_2_val;
      tmp->range_node = int_2;

      int_2 = tmp;
    } else
      int_2 = int_3;

    RangeOpNode<KType, AType> *result = new RangeOpNode<KType,AType>(int_2);
    result->op = sep_1;
    result->range_separator = sep_1_val;
    result->range_node = int_1;

    return result;
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
