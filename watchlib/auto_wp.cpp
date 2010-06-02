#define	WA_READ			1
#define	WA_WRITE		2
//#define	WA_EXEC			4
//#define	WA_TRAPAFTER	8

#include <vector>
#include <iostream>

namespace {
	struct watchpoint_t {
		int addr;
		int size;
		int flags;
	};
}

using std::cout;
using std::endl;
using std::vector;

namespace Hongyi_WatchPoint {
	class WatchPoint {
	public:
		//Constructors
		WatchPoint();
		WatchPoint(int target_addr, int target_size, int target_flags);
		WatchPoint(const WatchPoint& parameter);

		void add_watchpoint (int target_addr, int target_size, int target_flags);
		
//		void add_byte(int target_addr, int target_flags);
//		void add_range(int target_addr, int target_size, int target_flags);

//		void rm_byte(int target_addr);
//		void rm_range(int target_addr);

//		void watch_fault(int target_addr, int target_size, int target_flags);
		void watch_print();
	private:
		vector<watchpoint_t> wp;
	};
}

namespace {
	vector<watchpoint_t>::iterator search_address(int target_addr, vector<watchpoint_t>& wp) {
		vector<watchpoint_t>::iterator iter;
		for (i = wp.begin(); iter != wp.end(); i++) {
			if((*iter).addr > target_addr || (*iter).addr + (*iter).size > target_addr)
				break;
		}
		return iter;
	}
	
	bool watch_fault (int target_addr, int target_flags, const watchpoint_t& node) {
		return (target_addr >= node.addr && target_addr < node.addr + node.size && target.flags & node.flags);
	}
}
		

namespace Hongyi_WatchPoint{
	WatchPoint::WatchPoint() {
	}
	
	WatchPoint::WatchPoint(int target_addr, int target_size, int target_flags) {
		watchpoint_t temp = {target_addr, target_size, target_flags};
		wp.push_back(temp);
	}
	
	WatchPoint::WatchPoint(const WatchPoint& parameter) {
		wp.resize(parameter.wp.size());
		int i;
		for (i = 0; i < parameter.wp.size(); i++) {
			wp[i] = parameter.wp[i];
		}
	}
	
	void WatchPoint::watch_print() {
		int i;
		cout << "There are " << wp.size() << "watchpoints" << endl;
		for (i = 0; i < wp.size(); i++) {
			cout << "This is the " << i << "th watchpoint." <<endl;
			cout << "The watchpoint is at " << wp[i].addr << endl;
			cout << "The watchpoint has a size of " << wp[i].size << endl;
			if (wp[i].flags & WA_READ)
				cout << "The watchpoint is marked read." << endl;
			if (wp[i].flags & WA_WRITE)
				cout << "The watchpoint is marked write." << endl << endl;
		}
	}
	
	void WatchPoint::add_watchpoint(int target_addr, int target_size, int target_flags) {
		
		vector<watchpoint_t>::iterator iter;
		

/*
	//Below are local temperaray functions.
	void WatchPoint::add_watchpoint(int target_addr, int target_size, int target_flags) {
		watchpoint_t temp = {target_addr, target_size, target_flags};
		wp.push_back(temp);
	}
*/
}

int main() {
	using namespace Hongyi_WatchPoint;
	int target_addr;
	int target_size;
	int target_flags;
	WatchPoint watch;
	int i;
	
	for (i = 0; i < 5; i++) {
		target_addr = i*8;
		target_size = 4;
		target_flags = WA_READ | WA_WRITE;
		watch.add_watchpoint (target_addr, target_size, target_flags);
	}
	watch.watch_print();
	return 0;
}
