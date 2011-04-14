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

  Range<string,string> r1(dfl_val_1);
  r1.addRange(EQUAL, b_k, b_val);
  print_mapping(r1, a_k);
  print_mapping(r1, b_k);
  print_mapping(r1, c_k);
  do_traversal(r1);

  Range<string,string> r2(dfl_val_2);
  r2.addRange(LESS_THAN, c_k, c_val);
  print_mapping(r2, a_k);
  print_mapping(r2, b_k);
  print_mapping(r2, c_k);
  do_traversal(r2);

  Range<string,string> r3 = Range<string,string>::intersect(r1, r2, &MyTest::mywrapper, NULL);
  print_mapping(r3, a_k);
  print_mapping(r3, b_k);
  print_mapping(r3, c_k);
  do_traversal(r3);
}
