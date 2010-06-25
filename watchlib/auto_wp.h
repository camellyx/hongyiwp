//////////////////////////////////////////////////////
//													//
//		This is the headder file of the automatic	//
//		watchpoint implementation. It includes		//
//		automatic watchpoint adding and removing	//
//													//
//													//
//////////////////////////////////////////////////////
#ifndef deque_H
#include <deque>
#define deque_H
#endif
/*
namespace {
	#define	WA_READ			1
	#define	WA_WRITE		2
	
	struct watchpoint_t {
		int addr;
		int size;
		int flags;
	};
}
*/

using std::deque;

namespace Hongyi_WatchPoint {
	#define	WA_READ			1
	#define	WA_WRITE		2
	
	struct watchpoint_t {
		unsigned int addr;
		unsigned int size;
		unsigned int flags;
	};
	
	deque<watchpoint_t>::iterator search_address(unsigned int target_addr, deque<watchpoint_t>& wp);
	
	class WatchPoint {
	public:
		//Constructors
		WatchPoint();
		WatchPoint(unsigned int target_addr, unsigned int target_size, unsigned int target_flags);
		WatchPoint(const WatchPoint& parameter);

		
		void	add_read_wp		(unsigned int target_addr, unsigned int target_size);
		void	add_write_wp	(unsigned int target_addr, unsigned int target_size);
		
		void	rm_watch	(unsigned int target_addr, unsigned int target_size);
		void	rm_read		(unsigned int target_addr, unsigned int target_size);
		void	rm_write	(unsigned int target_addr, unsigned int target_size);
		
		unsigned int	watch_fault	(unsigned int target_addr, unsigned int target_size);
		//return: The number of how many watchpoints it touches within the range, regardless what kind of flags the watchpoint has.
		unsigned int	read_fault	(unsigned int target_addr, unsigned int target_size);
		//return: The number of how many *read* watchpoints it touches within the range.
		unsigned int	write_fault	(unsigned int target_addr, unsigned int target_size);
		//return: The number of how many *write* watchpoints it touches within the range.
		
		void	watch_print();
		
		void rm_watchpoint (unsigned int target_addr, unsigned int target_size, unsigned int target_flags);
		void add_watchpoint (unsigned int target_addr, unsigned int target_size, unsigned int target_flags);
		unsigned int general_fault (unsigned int target_addr, unsigned int target_size, unsigned int target_flags);
		deque<watchpoint_t> wp;
	private:
/*
		void rm_watchpoint (unsigned int target_addr, unsigned int target_size, unsigned int target_flags);
		void add_watchpoint (unsigned int target_addr, unsigned int target_size, unsigned int target_flags);
		unsigned int general_fault (unsigned int target_addr, unsigned int target_size, unsigned int target_flags);
		deque<watchpoint_t> wp;
*/
	};
}

