#include <memory>
#include <set>
#include <iostream>

using namespace std;
class A {
public:
    A(int aa, int vv) : a(aa), v(vv) {}
    // A (const A&) { }
    int a;
    int v;
};

std::set<std::weak_ptr<A>> setA;

int main () {
    set<string> strset;
    string s1="1234", s2="24567",s3="3456",s4="4567";
    strset.insert(s1);
    strset.insert(s2);
    strset.insert(s3);
    strset.insert(s4);
    cout<<*strset.begin()<<endl;
    string s5(std::move(*make_move_iterator(strset.begin())));
    cout<<s5<<endl<<*strset.begin()<<"endl"<<endl;
    string s6(std::move(s1));
    cout<<"s1 after move: "<<s1<<endl;
    string s7(std::move(*strset.begin()));
    cout<<s7<<':'<<*strset.begin()<<endl;
    // std::cout<<a1->v<<std::endl;



    return 0;
}