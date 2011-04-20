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

#include "range.hpp"
#include <string>
#include <iostream>
#include <stack>

using namespace std;

class MyTest{
public:
  MyTest() {MyTest::instance = this;}

  string* merger(const string *a, const string *b){
    string *s = new string("[merged '");
    s->append(*a).append("' with '").append(*b).append("']");
    cout << a << " " << b << ": " << *s << endl;
    return s;
  }

  static string* mywrapper(const string* a, const string* b, void* other){
    return instance->merger(a,b);
  }

private:
  static MyTest *instance;
};
MyTest* MyTest::instance = NULL;

void print_mapping(Range<string,string> &map, string *key){
  cout << "'" << (*key) << "' mapped to: '" << *(map.find(key)) << "'" << endl;
}
void print_mapping_int(Range<int,string> &map, int *key){
  cout << "'" << (*key) << "' mapped to: '" << *(map.find(key)) << "'" << endl;
}
void print_all_int(Range<int,string> &map){
  std::set<string*> ret_set = map.findAll();
  int i = 0;
  for (std::set<string*>::iterator iter = ret_set.begin();
       iter != ret_set.end();
       ++iter, ++i)
    cout << "action-" << i << ": " << (**iter) << endl;
}

void pr_indent(int i){ for(; i; --i) cout << " "; }
void cb_range(RangeOperator_t r, string *s, void *ptr){
  stack<int> *stk = (stack<int>*)ptr;
  int indent = stk->top();
  stk->pop();

  pr_indent(indent);
  cout << "RANGE: " << r << " for " << *s << endl;
  ++indent;
  stk->push(indent);
  stk->push(indent);
}
void cb_range_int(RangeOperator_t r, int *s, void *ptr){
  stack<int> *stk = (stack<int>*)ptr;
  int indent = stk->top();
  stk->pop();

  pr_indent(indent);
  cout << "RANGE: " << r << " for " << *s << endl;
  ++indent;
  stk->push(indent);
  stk->push(indent);
}
void cb_punt(RangeOperator_t r, std::map<string*,string*> m, void *ptr){
  stack<int> *stk = (stack<int>*)ptr;
  int indent = stk->top();
  stk->pop();

  pr_indent(indent);
  cout << "PUNT: " << endl;
  ++indent;
  for(std::map<string*,string*>::iterator iter = m.begin();
      iter != m.end();
      ++iter) {
    pr_indent(indent);
    cout << "{"<< *(iter->first) <<"} => {"<< *(iter->second) <<"}" << endl;
  }
  stk->push(indent);
}
void cb_punt_int(RangeOperator_t r, std::map<int*,string*> m, void *ptr){
  stack<int> *stk = (stack<int>*)ptr;
  int indent = stk->top();
  stk->pop();

  pr_indent(indent);
  cout << "PUNT: " << endl;
  ++indent;
  for(std::map<int*,string*>::iterator iter = m.begin();
      iter != m.end();
      ++iter) {
    pr_indent(indent);
    cout << "{"<< *(iter->first) <<"} => {"<< *(iter->second) <<"}" << endl;
  }
  stk->push(indent);
}
void cb_action(string *s, void *ptr){
  stack<int> *stk = (stack<int>*)ptr;
  int indent = stk->top();
  stk->pop();

  pr_indent(indent);
  cout << "ACTION: " << *s << endl;
}
void do_traversal(Range<string,string> &map){
  stack<int> stk;
  stk.push(0);
  map.traverse(cb_range, cb_punt, cb_action, &stk);
}

void do_traversal_int(Range<int,string> &map){
  stack<int> stk;
  stk.push(0);
  map.traverse(cb_range_int, cb_punt_int, cb_action, &stk);
}

int main(){
  MyTest t;

  string *dfl_val_1 = new string("DEFAULT1"); 
  string *dfl_val_2 = new string("DEFAULT2"); 
  string *a_k = new string("chiave_a");
  string *a_val = new string("valore_a");
  string *b_k = new string("chiave_b");
  string *b_val= new string("valore_b");
  string *c_k = new string("chiave_c");
  string *c_val= new string("valore_c");

  cout << "======== r1 ========" << endl;
  Range<string,string> r1(dfl_val_1);
  r1.addRange(EQUAL, b_k, b_val);
  print_mapping(r1, a_k);
  print_mapping(r1, b_k);
  print_mapping(r1, c_k);
  do_traversal(r1);

  cout << "======== r2 ========" << endl;
  Range<string,string> r2(dfl_val_2);
  r2.addRange(LESS_THAN, c_k, c_val);
  print_mapping(r2, a_k);
  print_mapping(r2, b_k);
  print_mapping(r2, c_k);
  do_traversal(r2);

  cout << "======== r3 ========" << endl;
  Range<string,string> r3 = Range<string,string>::intersect(r1, r2, &MyTest::mywrapper, NULL);
  print_mapping(r3, a_k);
  print_mapping(r3, b_k);
  print_mapping(r3, c_k);
  do_traversal(r3);

  cout << endl << endl;

  int v_a = 80, v_b = 1024, v_c = 32000;
  string *less_than_1024 = new string("lesser than 1024");
  string *eq_to_80 = new string("equal to 80");
  string *great_eq_32000 = new string("greater than or equal to 32000");
  string *dfl_val_3 = new string("DEFAULT3");

  cout << "======== rint1 ========" << endl;
  Range<int, string> rint1(dfl_val_1);
  rint1.addRange(LESS_THAN, &v_b, less_than_1024);
  print_mapping_int(rint1, &v_a);
  print_mapping_int(rint1, &v_b);
  print_mapping_int(rint1, &v_c);
  print_all_int(rint1);
  do_traversal_int(rint1);

  cout << "======== rint2 ========" << endl;
  Range<int, string> rint2(dfl_val_2);
  rint2.addRange(EQUAL, &v_a, eq_to_80);
  print_mapping_int(rint2, &v_a);
  print_mapping_int(rint2, &v_b);
  print_mapping_int(rint2, &v_c);
  print_all_int(rint2);
  do_traversal_int(rint2);

  cout << "======== rint3 ========" << endl;
  Range<int, string> rint3(dfl_val_3);
  rint3.addRange(GREAT_EQUAL_THAN, &v_c, great_eq_32000);
  print_mapping_int(rint3, &v_a);
  print_mapping_int(rint3, &v_b);
  print_mapping_int(rint3, &v_c);
  print_all_int(rint3);
  do_traversal_int(rint3);

  cout << "======== rint4 ========" << endl;
  Range<int,string> rint4 = Range<int,string>::intersect(rint1, rint2, &MyTest::mywrapper, NULL);
  print_mapping_int(rint4, &v_a);
  print_mapping_int(rint4, &v_b);
  print_mapping_int(rint4, &v_c);
  print_all_int(rint4);
  do_traversal_int(rint4);

  cout << "======== rint5 ========" << endl;
  Range<int,string> rint5 = Range<int,string>::intersect(rint4, rint3, &MyTest::mywrapper, NULL);
  print_mapping_int(rint5, &v_a);
  print_mapping_int(rint5, &v_b);
  print_mapping_int(rint5, &v_c);
  print_all_int(rint5);
  do_traversal_int(rint5);

  cout << "======== rint6 ========" << endl;
  Range<int,string> rint6 = Range<int,string>::intersect(rint2, rint3, &MyTest::mywrapper, NULL);
  print_mapping_int(rint6, &v_a);
  print_mapping_int(rint6, &v_b);
  print_mapping_int(rint6, &v_c);
  print_all_int(rint6);
  do_traversal_int(rint6);

  cout << "======== rint7 ========" << endl;
  Range<int,string> rint7 = Range<int,string>::intersect(rint6, rint1, &MyTest::mywrapper, NULL);
  print_mapping_int(rint7, &v_a);
  print_mapping_int(rint7, &v_b);
  print_mapping_int(rint7, &v_c);
  print_all_int(rint7);
  do_traversal_int(rint7);

  cout << "======== rint8 ========" << endl;
  Range<int,string> rint8 = Range<int,string>::intersect(rint1, rint6, &MyTest::mywrapper, NULL);
  print_mapping_int(rint8, &v_a);
  print_mapping_int(rint8, &v_b);
  print_mapping_int(rint8, &v_c);
  print_all_int(rint8);
  do_traversal_int(rint8);

  cout << "======== rint9 ========" << endl;
  Range<int,string> rint9 = Range<int,string>(rint8);
  print_mapping_int(rint9, &v_a);
  print_mapping_int(rint9, &v_b);
  print_mapping_int(rint9, &v_c);
  print_all_int(rint9);
  do_traversal_int(rint9);

  cout << "======== rint10_ptr ========" << endl;
  Range<int,string> *rint10_ptr = new Range<int,string>(rint8);
  print_mapping_int(*rint10_ptr, &v_a);
  print_mapping_int(*rint10_ptr, &v_b);
  print_mapping_int(*rint10_ptr, &v_c);
  print_all_int(*rint10_ptr);
  do_traversal_int(*rint10_ptr);
}
