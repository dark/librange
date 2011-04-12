#include "range.hpp"
#include <string>
#include <iostream>

using namespace std;

class MyTest{
public:
  string* m(string *a, string *b){
    cout << "lol" << endl;
    return a;
  }
};

string* mywrapper(string* a, string* b, void* other){
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
