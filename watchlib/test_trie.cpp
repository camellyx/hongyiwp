#ifndef vector_H
#include <deque>
#define vector_H
#endif

#include <iostream>
#include "auto_wp.h"

using namespace std;
using namespace Hongyi_WatchPoint;

int main () {
	unsigned int top_hit = 0;
	unsigned int mid_hit = 0;
	unsigned int bot_hit = 0;
	unsigned int target_addr;
	unsigned int target_size;
	
	WatchPoint<unsigned int, unsigned int> watch;
	target_addr = 0;
	target_size = 4194304;
	watch.add_watch_wp (target_addr, target_size);
	
	target_addr = 4198412;
	target_size = 4096;
	watch.add_watch_wp (target_addr, target_size);
	
	cout << "Below is for general_fault" << endl;
	if ( watch.general_fault(4194302, 8, 3, top_hit, mid_hit, bot_hit) ) {
		cout << "top_hit: " << top_hit << endl;
		cout << "mid_hit: " << mid_hit << endl;
		cout << "bot_hit: " << bot_hit << endl;
	}
	else
		cout << "OMG, big bug!!" << endl;
	return 0;
}

