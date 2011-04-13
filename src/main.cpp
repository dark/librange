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

using namespace std;

class MyTest{
public:
  string* m(const string *a, const string *b){
    cout << "lol" << endl;
    string *s = new string("");
    return s;
  }
};

string* mywrapper(const string* a, const string* b, void* other){
  MyTest *tmp = (MyTest*)other;
  return tmp->m(a,b);
}


int main(){
  string *a = new string("");
  string *b = new string("");
  MyTest t;

  string *me = new string("me");
  string *you= new string("you");

  Range<string,string> r1(a);
  r1.addRange(EQUAL, me, you);
  Range<string,string> r2(b);
  Range<string,string> r3 = Range<string,string>::intersect(r1, r2, &mywrapper, (void*)&t);
}
