//////////////////////////////////////////////////////
//													//
//		This is the headder file of the automatic	//
//		watchpoint implementation. It includes		//
//		automatic watchpoint adding and removing	//
//													//
//													//
//////////////////////////////////////////////////////
#ifndef VECTOR_H
#include <vector>
#define VECTOR_H
#endif

namespace {
	#define	WA_READ			1
	#define	WA_WRITE		2
	
	struct watchpoint_t {
		int addr;
		int size;
		int flags;
	};
}

using std::vector;

namespace Hongyi_WatchPoint {
	class WatchPoint {
	public:
		//Constructors
		WatchPoint();
		WatchPoint(int target_addr, int target_size, int target_flags);
		WatchPoint(const WatchPoint& parameter);

		
		void add_read_wp (int target_addr, int target_size);
		void add_write_wp (int target_addr, int target_size);
		
		void rm_watchpoint (int target_addr, int target_size);
		
		int watch_fault (int target_addr, int target_size);
		//return: The number of how many watchpoints it touches within the range, regardless what kind of flags the watchpoint has.
		int read_fault(int target_addr, int target_size);
		//return: The number of how many *read* watchpoints it touches within the range.
		int write_fault(int target_addr, int target_size);
		//return: The number of how many *write* watchpoints it touches within the range.
		
		void watch_print();
		
		void add_watchpoint (int target_addr, int target_size, int target_flags);
		int general_fault (int target_addr, int target_size, int target_flags);
	private:
		vector<watchpoint_t> wp;
	};
}

