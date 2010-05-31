#include <iostream>

using namespace std;

class OuterClass {
public:
	struct InnerStruct {
		int a;
		int b;
	} hello;
//	InnerClass hello;
};

int main () {
	OuterClass hi;
	hi.hello.a = 1;
	hi.hello.b = 2;
	cout << "This is the struct version!" << endl;
	cout << "Here is the value of hi.hello.a and it should be 1" << endl;
	cout << "Here is the value of hi.hello.b and it should be 2" << endl;
	return 0;
}

