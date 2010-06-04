#include <iostream>
#include <vector>

using namespace std;

int main () {
	vector<int> vec;
	int i;
	for (i = 0; i < 6; i++)
		vec.push_back(i);
	for (i = 0; i < 6; i++)
		cout << "Vector[" << i << "] = " << vec[i] << endl;
	vector<int>::iterator iter;
	iter = vec.begin();
	iter = iter + 2;
	cout << "iterator is now at " << (*iter) << endl;
	vector<int>::iterator iter_del = vec.begin() + 1;
	vec.erase(iter_del);
	cout << "I've delete vector[1]" << endl;
	for (i = 0; i < vec.size(); i++)
		cout << "Vector[" << i << "] = " << vec[i] << endl;
	cout << "iterator is now at " << (*iter) << endl;
	return 0;
}
	
