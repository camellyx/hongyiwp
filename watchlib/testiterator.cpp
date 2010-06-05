#include <iostream>
#include <vector>
#include <cstddef>

struct watchpoint_t {
	int addr;
	int size;
	int flags;
};

//using namespace std;
using std::cout;
using std::endl;
using std::vector;

vector<watchpoint_t>::iterator search_address(int target_addr, vector<watchpoint_t> & wp) {
	vector<watchpoint_t>::iterator iter;
	for (iter = wp.begin(); iter != wp.end(); iter++) {
		if((*iter).addr > target_addr || (*iter).addr + (*iter).size > target_addr)
			break;
	}
	return iter;
}

bool addr_covered (int target_addr, const watchpoint_t& node) {
	return (target_addr >= node.addr && target_addr < node.addr + node.size);
}

bool watch_fault (int target_addr, int target_flags, const watchpoint_t& node) {
	return (target_addr >= node.addr && target_addr < node.addr + node.size && target_flags & node.flags);
}

int main () {
	vector<watchpoint_t> wp;
	watchpoint_t temp;
	int i;
	for (i = 0; i < 5; i++) {
		temp.addr = (i+1)*10;
		temp.size = 5;
		temp.flags = 3;
		wp.push_back(temp);
	}
	
	vector<watchpoint_t>::iterator iter;
/*
	for (iter = wp.begin(); iter != wp.end(); iter++) {
		if((*iter).addr > 22 || (*iter).addr + (*iter).size > 22)
			break;
	}
*/
	int target_addr;
	target_addr = 0;
	iter = search_address(target_addr, wp);
	if (iter != wp.end()) {
		cout << "The result addr is " << iter->addr << endl;
		cout << "The result size is " << iter->size << endl;
		cout << "The reuslt flag is " << iter->flags << endl;
		if (addr_covered(target_addr, (*iter) ) )
			cout << "The target_address is covered" << endl;
		else
			cout << "The target_address is not covered" << endl;
	}
	else
		cout << "The result is Beyond the data structure" << endl;
	return 0;
}

