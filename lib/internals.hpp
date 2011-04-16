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
  virtual Node_t getType() const = 0;
  // must return NULL on lookup failure
  virtual AType* find(KType* key) const = 0;
  virtual void traverse(range_callback_func_t range_callback, punt_callback_func_t punt_callback, action_callback_func_t action_callback, void *extra_info) const = 0;
};


template <class KType, class AType>
class ActionNode : public TreeNode<KType,AType>
{
  friend class TreeMerger<KType, AType>;

  typedef void(*range_callback_func_t)(RangeOperator_t, KType*, void*);
  typedef void(*punt_callback_func_t)(RangeOperator_t, std::map<KType*,AType*>, void*);
  typedef void(*action_callback_func_t)(AType*, void*);

public:
  ActionNode(AType *action) : action(action){}
  Node_t getType() const { return ACTION; }
  AType* find(KType *key) const {return action;}
  void traverse(range_callback_func_t range_callback, punt_callback_func_t punt_callback, action_callback_func_t action_callback, void *extra_info) const
  { if (action_callback) (*action_callback)(action, extra_info); }

private:
  AType *action;
};


template <class KType, class AType>
class OpNode : public TreeNode<KType,AType>
{
  friend class TreeMerger<KType, AType>;

public:
  static OpNode* buildOpNode(AType *dfl_action, RangeOperator_t op, KType *key, AType *cond_action);
  virtual void addRange(RangeOperator_t op, KType *key, AType *cond_action) = 0;
  inline RangeOperator_t getOp() const {return op;}
  inline RangeOperator_t getNormalizedOp() const {
    if (op == LESS_THAN || op == GREAT_EQUAL_THAN)
      return LESS_THAN;
    if (op == LESS_EQUAL_THAN || op == GREAT_THAN)
      return LESS_EQUAL_THAN;
    return op;
  }

protected:
  const TreeNode<KType,AType> *dfl_node;
  RangeOperator_t op;
};


template <class KType, class AType>
class RangeOpNode : public OpNode<KType,AType>
{
  friend class TreeMerger<KType, AType>;

  typedef void(*range_callback_func_t)(RangeOperator_t, KType*, void*);
  typedef void(*punt_callback_func_t)(RangeOperator_t, std::map<KType*,AType*>, void*);
  typedef void(*action_callback_func_t)(AType*, void*);

public:
  RangeOpNode(AType *outside_action)
    : range_separator(NULL), range_node(NULL)
  {
    this->dfl_node = new ActionNode<KType,AType>(outside_action);
  }

  Node_t getType() const { return RANGE; }

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

  AType* find(KType* key) const {
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

  void traverse(range_callback_func_t range_callback, punt_callback_func_t punt_callback, action_callback_func_t action_callback, void *extra_info) const
  {
    if (range_callback)
      (*range_callback)(this->op, range_separator,  extra_info);

    this->range_node->traverse(range_callback, punt_callback, action_callback, extra_info);
    this->dfl_node->traverse(range_callback, punt_callback, action_callback, extra_info);
  }

private:
  KType *range_separator;
  TreeNode<KType,AType> *range_node;

  RangeOpNode(const TreeNode<KType,AType> *dfl_node)
    : range_separator(NULL), range_node(NULL)
  {
    this->dfl_node = dfl_node; 
  }

  inline const TreeNode<KType,AType>* left_interval() const {
    if (this->op == EQUAL || this->op == INVALID)
      abort(); // something broke
    if (this->op == LESS_THAN || this->op == LESS_EQUAL_THAN)
      return range_node;
    return this->dfl_node;
  }

  inline const TreeNode<KType,AType>* right_interval() const {
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
  friend class TreeMerger<KType, AType>;

  typedef void(*range_callback_func_t)(RangeOperator_t, KType*, void*);
  typedef void(*punt_callback_func_t)(RangeOperator_t, std::map<KType*,AType*>, void*);
  typedef void(*action_callback_func_t)(AType*, void*);

public:
  PunctOpNode(AType *default_action)
  {
    this->dfl_node = new ActionNode<KType,AType>(default_action);
  }

  Node_t getType() const { return PUNCTUAL; }

  void addRange(RangeOperator_t op, KType *key, AType *cond_action)
  {
    if (op != EQUAL)
      abort();

    addPuntAction(key, cond_action);
  }

  AType* find(KType* key) const {
    typename std::map<KType*,AType*>::const_iterator i = others.find(key);
    if (i == others.end())
      return this->dfl_node->find(key);
    return i->second;
  }

  void traverse(range_callback_func_t range_callback, punt_callback_func_t punt_callback, action_callback_func_t action_callback, void *extra_info) const
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

  PunctOpNode(const TreeNode<KType,AType> *dfl_node)
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
  typedef AType*(*merger_func_t)(const AType*, const AType*, void*);

public:
  static TreeNode<KType, AType>* merge(const TreeNode<KType, AType> *a, const TreeNode<KType, AType> *b,
                                       merger_func_t merger, void *extra_info,
                                       const KType *bound_low, const bool bl_incl,
                                       const KType *bound_high, const bool bh_incl)
  {
    Node_t a_type = a->getType();
    Node_t b_type = b->getType();

    // handle immediately the easiest cases
    if (a_type == ACTION && b_type == ACTION) {
      const ActionNode<KType, AType> *a_prom_action = dynamic_cast<const ActionNode<KType, AType>*>(a);
      const ActionNode<KType, AType> *b_prom_action = dynamic_cast<const ActionNode<KType, AType>*>(b);
      if (!a_prom_action || !b_prom_action) abort(); // something broke
      AType *m = (*merger)(a_prom_action->action, b_prom_action->action, extra_info);
      return new ActionNode<KType, AType>(m);
    }

    // I prefer to have more 'complex' types in 'a' rather than in 'b',
    // so swap them if necessary
    if ( (b_type == RANGE && a_type != RANGE) ||
         (b_type == PUNCTUAL && a_type == ACTION) )
      return merge(b, a, merger, extra_info, bound_low, bl_incl, bound_high, bh_incl);

    // handle remaining cases
    if (a_type == RANGE) {
      // the left node is a RangeOpNode
      const RangeOpNode<KType, AType> *a_prom_range = dynamic_cast<const RangeOpNode<KType, AType>*>(a);
      if (!a_prom_range) abort(); // something broke

      RangeOpNode<KType, AType> *result_range = NULL;
      switch(b_type){
      case RANGE:
        {
          // the right node is a RangeOpNode
          const RangeOpNode<KType, AType> *b_prom_range = dynamic_cast<const RangeOpNode<KType, AType>*>(b);
          if (!b_prom_range) abort(); // something broke
          result_range = merge_range_range(a_prom_range, b_prom_range, merger, extra_info,
                                           bound_low, bl_incl, bound_high, bh_incl);
          break;
        }

      case PUNCTUAL:
        {
          // the right node is a PunctOpNode
          const PunctOpNode<KType, AType> *b_prom_punct = dynamic_cast<const PunctOpNode<KType, AType>*>(b);
          if (!b_prom_punct) abort(); // something broke
          result_range = merge_range_punct(a_prom_range, b_prom_punct, merger, extra_info,
                                           bound_low, bl_incl, bound_high, bh_incl);
          break;
        }

      case ACTION:
        {
          // the right node is a ActionNode
          const RangeOperator_t a_op = a_prom_range->getOp();
          TreeNode<KType, AType> *dfl_node = NULL;
          TreeNode<KType, AType> *range_node = NULL;
          if (a_op == LESS_THAN || a_op == LESS_EQUAL_THAN) {
            // dfl_node must be checked for compatibility against the
            // upper bound and range_node against the lower one
            if(is_out_of_high_bound(a_prom_range->range_separator,
                                    bound_high, bh_incl))
              dfl_node = NULL;
            else
              dfl_node = merge(a_prom_range->dfl_node, b, merger, extra_info,
                               a_prom_range->range_separator, a_op == LESS_THAN,
                               bound_high, bh_incl);

            if(is_out_of_low_bound(a_prom_range->range_separator,
                                   bound_low, bl_incl))
              range_node = NULL;
            else
              range_node = merge(a_prom_range->range_node, b, merger, extra_info,
                                 bound_low, bl_incl,
                                 a_prom_range->range_separator, a_op == LESS_EQUAL_THAN);
          } else {
            // ... the opposite :)
            if(is_out_of_low_bound(a_prom_range->range_separator,
                                   bound_low, bl_incl))
              dfl_node = NULL;
            else
              dfl_node = merge(a_prom_range->dfl_node, b, merger, extra_info,
                               bound_low, bl_incl,
                               a_prom_range->range_separator, a_op == GREAT_THAN);

            if(is_out_of_high_bound(a_prom_range->range_separator,
                                    bound_high, bh_incl))
              range_node = NULL;
            else
              range_node = merge(a_prom_range->range_node, b, merger, extra_info,
                                 a_prom_range->range_separator, a_op == GREAT_EQUAL_THAN,
                                 bound_high, bh_incl);
          }

          // only one among dfl_node or range_node can be out of bounds
          if (dfl_node == NULL)
            return range_node;
          if (range_node == NULL)
            return dfl_node;

          result_range = new RangeOpNode<KType, AType>(dfl_node);
          result_range->op = a_prom_range->op;
          result_range->range_separator = a_prom_range->range_separator;
          result_range->range_node = range_node;

          break;
        }
      }

      return result_range;
    } else if (a_type == PUNCTUAL) {
      // the left node is a PunctOpNode
      const PunctOpNode<KType, AType> *a_prom_punct = dynamic_cast<const PunctOpNode<KType, AType>*>(a);
      if (!a_prom_punct) abort(); // something broke

      switch(b_type){
      case PUNCTUAL:
        {
          // the right node is a PunctOpNode
          const PunctOpNode<KType, AType> *b_prom_punct = dynamic_cast<const PunctOpNode<KType, AType>*>(b);
          if (!b_prom_punct) abort(); // something broke
          return merge_punct_punct(a_prom_punct, b_prom_punct, merger, extra_info,
                                   bound_low, bl_incl, bound_high, bh_incl);
          break;
        }

      case ACTION:
        { 
          // the right node is a ActionNode
          PunctOpNode<KType, AType> *result_punct = NULL;

          TreeNode<KType, AType> *new_dfl_node = merge(a_prom_punct->dfl_node, b, merger, extra_info, bound_low, bl_incl, bound_high, bh_incl);
          const AType *b_action = dynamic_cast<const ActionNode<KType, AType>*>(b)->action;

          for(typename std::map<KType*,AType*>::const_iterator i = a_prom_punct->others.begin();
              i != a_prom_punct->others.end();
              ++i) {
            if ( is_out_of_low_bound(i->first, bound_low, bl_incl)
                 || is_out_of_high_bound(i->first, bound_high, bh_incl) )
              // check for boundaries
              continue;

            if (!result_punct) {
              result_punct = new PunctOpNode<KType, AType>(new_dfl_node);
              result_punct->op = EQUAL;
            }
            result_punct->others[i->first]=(*merger)(i->second, b_action, extra_info);
          }

          if (!result_punct) // boundaries prevented me from adding any value to result_punct
            return new_dfl_node;

          return result_punct;
        }

      default: abort(); // something went wrong
      } // end of: switch(b_type)

      abort(); // I should have returned something above
    } // end of: else if (a_type == PUNCTUAL)
    else {
      abort(); // something broke in the comparisons above
    }
  }


private:
  static RangeOpNode<KType, AType>* merge_range_range(const RangeOpNode<KType, AType> *a,
                                                      const RangeOpNode<KType, AType> *b,
                                                      merger_func_t merger, void *extra_info,
                                                      const KType *bound_low, const bool bl_incl,
                                                      const KType *bound_high, const bool bh_incl)
  {
    KType *a_separator = a->range_separator;
    KType *b_separator = b->range_separator;

    // except when a_separator == b_separator, the intersection
    // between two ranges gives three intervals and two separators
    TreeNode<KType, AType> *int_1=NULL, *int_2=NULL, *int_3=NULL;
    RangeOperator_t sep_1=INVALID, sep_2=INVALID;
    KType *sep_1_val=NULL, *sep_2_val=NULL;
    if(*a_separator == *b_separator) {
      // A quick mental survey led me to believe that can be no
      // boundaries issues in this subcase. Boundaries only have to be
      // propagated down in the recursion chain.
      sep_1 = a->getNormalizedOp();
      sep_2 = b->getNormalizedOp();
      sep_1_val = sep_2_val = a_separator;
      int_1 = merge(a->left_interval(),
                    b->left_interval(),
                    merger, extra_info,
                    bound_low, bl_incl, sep_1_val,
                    sep_1 == LESS_EQUAL_THAN && sep_2 == LESS_EQUAL_THAN);
      int_3 = merge(a->right_interval(),
                    b->right_interval(),
                    merger, extra_info,
                    sep_2_val, sep_1 == LESS_THAN && sep_2 == LESS_THAN,
                    bound_high, bh_incl);
      if(a->getNormalizedOp() != b->getNormalizedOp()) {
        // there is a small "gap" between the intervals (as in '<x' and '>x')
        // or they are overlapped ('<=x' and '>=x')
        // handle both cases here
        sep_1 = LESS_THAN;
        sep_2 = LESS_EQUAL_THAN;
        int_2 = merge((sep_1 == LESS_THAN? a->right_interval() : a->left_interval() ),
                      (sep_1 == LESS_THAN? a->left_interval() : a->right_interval() ),
                      merger, extra_info,
                      sep_1_val, true, sep_2_val, true);
      } 
    } else {
      // *a_separator != *b_separator
      const RangeOpNode<KType, AType> *range_left = (*a_separator < *b_separator? a : b);
      const RangeOpNode<KType, AType> *range_right = (*a_separator < *b_separator? b : a);
      sep_1 = range_left->getNormalizedOp();
      sep_2 = range_right->getNormalizedOp();
      sep_1_val = range_left->range_separator;
      sep_2_val = range_right->range_separator;

      int_1 = (is_out_of_low_bound(sep_1_val, bound_low,
                                   sep_1 == LESS_EQUAL_THAN && bl_incl)
               ? NULL :
               merge(range_left->left_interval(),
                     range_right->left_interval(),
                     merger, extra_info,
                     bound_low, bl_incl, sep_1_val, sep_1 == LESS_EQUAL_THAN)
        );
      int_2 = merge(range_left->right_interval(),
                    range_right->left_interval(),
                    merger, extra_info,
                    sep_1_val, sep_1 == LESS_THAN, sep_2_val, sep_2 == LESS_EQUAL_THAN);
      int_3 = (is_out_of_high_bound(sep_2_val, bound_low,
                                    sep_2 == LESS_THAN && bh_incl)
               ? NULL :
               merge(range_left->right_interval(),
                     range_right->right_interval(),
                     merger, extra_info,
                     sep_2_val, sep_2 == LESS_THAN, bound_high, bh_incl)
        );
    }

    // From the above, only one among int_1, int_2 or int_3 can be
    // NULL. I'll act accordingly.
        
    // if int_2 == NULL, sep_2 will be ignored
    if (int_2) {
      if (int_3) {
        RangeOpNode<KType,AType> *tmp = new RangeOpNode<KType,AType>(int_3);
        tmp->op = sep_2;
        tmp->range_separator = sep_2_val;
        tmp->range_node = int_2;

        int_2 = tmp;
      }
      // else do nothing, int_2 is good as is
    } else
      int_2 = int_3;

    RangeOpNode<KType, AType> *result;
    if (int_1) {
      result = new RangeOpNode<KType,AType>(int_2);
      result->op = sep_1;
      result->range_separator = sep_1_val;
      result->range_node = int_1;
    }
    else {
      // this is always legal because of the code paths above and the
      // fact that only one among int_1, int_2 and int_3 can be NULL
      result = dynamic_cast<RangeOpNode<KType, AType> *>(int_2);
    }

    return result;
  }

  static RangeOpNode<KType, AType>* merge_range_punct(const RangeOpNode<KType, AType> *a,
                                                      const PunctOpNode<KType, AType> *b,
                                                      merger_func_t merger, void *extra_info,
                                                      const KType *bound_low, const bool bl_incl,
                                                      const KType *bound_high, const bool bh_incl)
  {
#warning update to account boundaries
    KType *a_separator = a->range_separator;
    const RangeOperator_t a_op = a->getOp();
    const RangeOperator_t a_norm_op = a->getNormalizedOp();

    // The result of the merge will be a RangedOpNode at the top, with TreeNode(s)
    // below it (their type depends on the actual code path taken here).
    RangeOpNode<KType, AType> *result = NULL;
    // While building, I keep temporary variables of type PunctOpNode for the children
    PunctOpNode<KType, AType> *tmp_child_left = NULL, *tmp_child_right=NULL;

    // separate left-hand punctual values from right-hand ones
    bool scanning_left_side = true;
    for(typename std::map<KType*,AType*>::const_iterator i = b->others.begin();
        i != b->others.end();
        ++i) {
      if (scanning_left_side && ( a_norm_op == LESS_THAN ?
                                  *(i->first) < *a_separator :
                                  *(i->first) <= *a_separator ))
      {
        // the current punctual value must go in the left child
        if (!tmp_child_left) {
          tmp_child_left = new PunctOpNode<KType, AType>(b->dfl_node);
          tmp_child_left->op = EQUAL;
        }
        tmp_child_left->addPuntAction(i->first, i->second);
      } else {
        // the current punctual value must go in the right child
        scanning_left_side = false;
        if (!tmp_child_right) {
          tmp_child_right = new PunctOpNode<KType, AType>(b->dfl_node);
          tmp_child_right->op = EQUAL;
        }
        tmp_child_right->addPuntAction(i->first, i->second);
      }
    }

    // Actually create the children. Must take care of two things:
    // 1) the orientation of the parent RangeOpNode
    // 2) whether if any of the tmp children is empty
    TreeNode<KType, AType> *child_left = merge((a_op == LESS_THAN || a_op == LESS_EQUAL_THAN ?
                                                a->range_node : a->dfl_node ),
                                               (tmp_child_left? tmp_child_left : b->dfl_node),
                                               merger, extra_info,
                                               bound_low, bl_incl, bound_high, bh_incl);
    TreeNode<KType, AType> *child_right = merge((a_op == LESS_THAN || a_op == LESS_EQUAL_THAN ?
                                                 a->dfl_node : a->range_node ),
                                                (tmp_child_right? tmp_child_right : b->dfl_node),
                                                merger, extra_info,
                                                bound_low, bl_incl, bound_high, bh_incl);

    result = new RangeOpNode<KType, AType>(child_right);
    result->op = a_norm_op;
    result->range_separator = a_separator;
    result->range_node = child_left;

    return result;
  }

  static TreeNode<KType, AType>* merge_punct_punct(const PunctOpNode<KType, AType> *a,
                                                   const PunctOpNode<KType, AType> *b,
                                                   merger_func_t merger, void *extra_info,
                                                   const KType *bound_low, const bool bl_incl,
                                                   const KType *bound_high, const bool bh_incl)
  {
    const ActionNode<KType, AType> *a_dfl = dynamic_cast<const ActionNode<KType, AType>*>(a->dfl_node);
    const ActionNode<KType, AType> *b_dfl = dynamic_cast<const ActionNode<KType, AType>*>(b->dfl_node);
    if (!a_dfl || !b_dfl) abort(); // something broke

    TreeNode<KType, AType> *merged_dfl = merge(a->dfl_node, b->dfl_node, merger, extra_info, bound_low, bl_incl, bound_high, bh_incl);
    PunctOpNode<KType, AType> *result = new PunctOpNode<KType, AType>(merged_dfl);
    result->op = EQUAL;
    
    typename std::map<KType*,AType*>::const_iterator a_iter = a->others.begin();
    typename std::map<KType*,AType*>::const_iterator b_iter = b->others.begin();
    for (; a_iter != a->others.end() && b_iter != b->others.end() ; ) {
      // need to account both the lower and the higher bounds in all subcases
      if( *(a_iter->first) < *(b_iter->first) ) {
        if(!is_out_of_low_bound(a_iter->first, bound_low, bl_incl))
          result->others[a_iter->first]=(*merger)(a_iter->second, b_dfl->action, extra_info);

        if(is_out_of_high_bound(a_iter->first, bound_high, bh_incl))
          break; // all the following in both 'a' and 'b' will be out of upper bound

        ++a_iter; // advance the iterator
      } else if ( *(a_iter->first) > *(b_iter->first) ) {
        if(!is_out_of_low_bound(b_iter->first, bound_low, bl_incl))
          result->others[b_iter->first]=(*merger)(a_dfl->action, b_iter->second, extra_info);

        if(is_out_of_high_bound(b_iter->first, bound_high, bh_incl))
          break; // all the following in both 'a' and 'b' will be out of upper bound

        ++b_iter; // advance the iterator
      } else { // if *(a_iter->first) == *(b_iter->first) )
        if(!is_out_of_low_bound(a_iter->first, bound_low, bl_incl))
          result->others[a_iter->first]=(*merger)(a_iter->second, b_iter->second, extra_info);

        if(is_out_of_high_bound(a_iter->first, bound_high, bh_incl))
          break; // all the following in both 'a' and 'b' will be out of upper bound

        ++a_iter;
        ++b_iter;
      }
    }

    // add the remaining mappings
    for (; !is_out_of_high_bound(a_iter->first, bound_high, bh_incl)
           && a_iter != a->others.end(); ++a_iter)
      result->others[a_iter->first]=(*merger)(a_iter->second, b_dfl->action, extra_info);
    for (; !is_out_of_high_bound(b_iter->first, bound_high, bh_incl)
           && b_iter != b->others.end(); ++b_iter)
      result->others[b_iter->first]=(*merger)(a_dfl->action, b_iter->second, extra_info);

    if(result->others.size() == 0) {
      // everything was out of bound
      delete result;
      return merged_dfl;
    }

    return result;
  }

  inline static bool is_out_of_high_bound(const KType *val, const KType *high_bound,
                                          bool bh_included)
  {
    return (high_bound &&
            (bh_included ? *val > *high_bound : *val >= *high_bound)
      );
  }

  inline static bool is_out_of_low_bound(const KType *val, const KType *low_bound,
                                          bool bl_included)
  {
    return (low_bound &&
            (bl_included ? *val < *low_bound : *val <= *low_bound)
      );
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
