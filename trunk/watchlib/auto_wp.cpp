#define	WA_READ			1
#define	WA_WRITE		2
#define	WA_EXEC			4
#define	WA_TRAPAFTER	8

#include <vector>
#include <iostream>

struct {
	int addr;
	int size;
	int flag;
} watchpoint_t;

using std::cout;
using std::endl;
using std::vector<watchpoint_t>;
using std::vector<watchpoint_t>::iterator;

namespace Hongyi_WatchPoint {
	class WatchPoint ｛
	public:
		//Constructors
		WatchPoint();
		WatchPoint(int target_addr, int target_size, int target_flags);
		WatchPoint(WatchPoint parameter);
		
		void add_byte(int target_addr, int target_flags);
		void add_range(int target_addr, int target_size, int target_flags);

		void rm_byte(int target_addr);
		void rm_range(int target_addr);

		void watch_fault(int target_addr, int target_size, int target_flags);
		void watch_print();
	private:
		struct watchpoint_t {
			int addr;
			int size;
			int flags;
		};
		vector<watchpoint_t> wp;
	};
}

namespace {
	iterator 

namespace Hongyi_WatchPoint{
	WatchPoint::WatchPoint() {
	}
	
	WatchPoint::WatchPoint(int target_addr, int target_size, int target_flags) {
		watchpoint_t temp = {target_addr, target_size, target_flags};
		wp.push_back(temp);
	}
	
	WatchPoint::WatchPoint(WatchPoint parameter) {
		wp.resize(parameter.wp.size());
		int i;
		for (i = 0; i < parameter.wp.size(); i++) {
			wp.push_back(parameter.wp[i]);
		}
	}
