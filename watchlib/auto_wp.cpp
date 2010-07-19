#ifndef WP_H
#include <deque>
#include <iostream>
#define WP_H
#endif

#include "auto_wp.h"

/*
namespace {
	#define	WA_READ			1
	#define	WA_WRITE		2
	
	struct watchpoint_t<ADDRESS, FLAGS> {
		int addr;
		int size;
		int flags;
	};
}
*/

using std::cout;
using std::endl;
using std::deque;
using Hongyi_WatchPoint::watchpoint_t;

/*
namespace Hongyi_WatchPoint {
	class WatchPoint {
	public:
		//Constructors
		WatchPoint();
		WatchPoint(int target_addr, int target_size, int target_flags);
		WatchPoint(const WatchPoint& parameter);

		
		void	add_read_wp		(int target_addr, int target_size);
		void	add_write_wp	(int target_addr, int target_size);
		
		void	rm_watch	(int target_addr, int target_size);
		void	rm_read		(int target_addr, int target_size);
		void	rm_write	(int target_addr, int target_size);
		
		int		watch_fault	(int target_addr, int target_size);
		//return: The number of how many watchpoints it touches within the range, regardless what kind of flags the watchpoint has.
		int		read_fault	(int target_addr, int target_size);
		//return: The number of how many *read* watchpoints it touches within the range.
		int		write_fault	(int target_addr, int target_size);
		//return: The number of how many *write* watchpoints it touches within the range.
		
		void	watch_print();
		
		void rm_watchpoint (int target_addr, int target_size, int target_flags);
		void add_watchpoint (int target_addr, int target_size, int target_flags);
		int general_fault (int target_addr, int target_size, int target_flags);
	private:
		deque<watchpoint_t<ADDRESS, FLAGS> > wp;
	};
}
*/

namespace {
	template<class ADDRESS, class FLAGS>
	bool addr_covered (ADDRESS target_addr, const watchpoint_t<ADDRESS, FLAGS>& node);
	
	template<class FLAGS>
	bool flag_inclusion (FLAGS target_flags, FLAGS container_flags);
	
	template<class ADDRESS, class FLAGS>
	typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator search_address(ADDRESS target_addr, deque<watchpoint_t<ADDRESS, FLAGS> >& wp);

	template<class ADDRESS, class FLAGS>
	bool addr_covered (ADDRESS target_addr, const watchpoint_t<ADDRESS, FLAGS>& node) {
		return (target_addr >= node.addr && target_addr < node.addr + node.size);
	}
	
	template <class FLAGS>
	bool flag_inclusion (FLAGS target_flags, FLAGS container_flags) {
		return ( ( (target_flags ^ container_flags) & container_flags) == (target_flags ^ container_flags) );
	}
	
	template <class ADDRESS, class FLAGS>
	typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator search_address(ADDRESS target_addr, deque<watchpoint_t<ADDRESS, FLAGS> >& wp) {
		int size = wp.size();
		if (size == 0)
			return wp.end();
		bool find = false;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg, end, mid;
		beg = wp.begin();
		end = wp.end() - 1;
		
		if (end->addr < target_addr)
			return wp.end();
		if (beg->addr > target_addr)
			return wp.begin();
			
		mid = beg + size / 2;
		find = addr_covered (target_addr, *mid);
		while(beg < end && !find) {
			
			if (target_addr < mid->addr) {
				end = mid - 1;
				size = size / 2;
				mid = beg + size / 2;
			}
			else {
				if (size % 2)
					beg = mid + 1;
				else
					beg = mid;
				size = size / 2;
				mid = beg + size / 2;
			}
			
			find = addr_covered(target_addr, *mid);
		}
		
		if (find || target_addr < mid->addr)
			return mid;
		else
			return mid + 1;
	}
}
		

namespace Hongyi_WatchPoint{
	
	template <class ADDRESS, class COUNT, class FLAGS>
	WatchPoint<ADDRESS, COUNT, FLAGS>::WatchPoint() {
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	WatchPoint<ADDRESS, COUNT, FLAGS>::WatchPoint(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		watchpoint_t<ADDRESS, FLAGS> temp = {target_addr, target_size, target_flags};
		wp.push_back(temp);
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	WatchPoint<ADDRESS, COUNT, FLAGS>::WatchPoint(const WatchPoint& parameter) {
		wp.resize((int)parameter.wp.size());
		int i;
		for (i = 0; i < parameter.wp.size(); i++) {
			wp[i] = parameter.wp[i];
		}
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	void WatchPoint<ADDRESS, COUNT, FLAGS>::watch_print() {
		int i;
		cout << "There are " << wp.size() << " watchpoints" << endl;
		for (i = 0; i < (int)wp.size(); i++) {
			cout << "This is the " << i << "th watchpoint." <<endl;
			cout << "The watchpoint is at " << wp[i].addr << endl;
			cout << "The watchpoint has a size of " << wp[i].size << endl;
			if (wp[i].flags & WA_READ)
				cout << "||READ||" << endl;
			if (wp[i].flags & WA_WRITE)
				cout << "||WRITE||" << endl;
		}
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	void WatchPoint<ADDRESS, COUNT, FLAGS>::add_read_wp (ADDRESS target_addr, ADDRESS target_size) {
		add_watchpoint (target_addr, target_size, WA_READ);
		return;
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	void WatchPoint<ADDRESS, COUNT, FLAGS>::add_write_wp (ADDRESS target_addr, ADDRESS target_size) {
		add_watchpoint (target_addr, target_size, WA_WRITE);
		return;
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	void WatchPoint<ADDRESS, COUNT, FLAGS>::add_watchpoint(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		if (target_size == 0)
			return;
		watchpoint_t<ADDRESS, FLAGS> insert_t = {0, 0, 0};
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator start_iter;//This one is used only for merging the front wp nodes.
		iter = search_address(target_addr, wp);
		
		if (iter == wp.end() ) {
			if (iter != wp.begin() ) {//We'll need to check if there is some wp ahead of iter and if there is then we need to decide whether merge the two.
				start_iter = iter - 1;
				if (start_iter->addr + start_iter->size == target_addr && start_iter->flags == target_flags) {//Merge condition.
					start_iter->size = start_iter->size + target_size;//Merge by enlarging the former wp node.
					return;
				}
			}
			insert_t.addr = target_addr;
			insert_t.size = target_size;
			insert_t.flags = target_flags;
			wp.push_back(insert_t);
			return;
		}
		if (addr_covered (target_addr, (*iter) ) ) {
			if (iter->addr == target_addr) {//We will only need to consider merging if this is true.
				if (iter != wp.begin() ) {
					start_iter = iter - 1;
					if(start_iter->addr + start_iter->size == target_addr && start_iter->flags == (iter->flags | target_flags) ) {//We will have to merge the two node.
						insert_t.addr = start_iter->addr;
						insert_t.flags = start_iter->flags;
						if (iter->addr + iter->size > target_addr + target_size && ( (target_flags ^ iter->flags) & iter->flags) != (target_flags ^ iter->flags) ) {
							// If the "add" is too short and the adding range ends within a wp. Test split as well
							iter->size = iter->size - target_size;
							iter->addr = target_addr + target_size;
							insert_t.size = target_size + start_iter->size;
							wp.insert(iter, insert_t);
							return;
						}
						insert_t.size = start_iter->size + iter->size;
						iter = wp.erase(start_iter);
						iter = wp.erase(iter);
					}
					else {
						insert_t.addr = iter->addr;
						insert_t.flags = iter->flags | target_flags;
						if (iter->addr + iter->size > target_addr + target_size && !flag_inclusion (target_flags, iter->flags) ) {
							iter->size = iter->size - target_size;
							iter->addr = target_addr + target_size;
							insert_t.size = target_size;
							wp.insert(iter, insert_t);
							return;
						}
						insert_t.size = iter->size;
						iter = wp.erase(iter);//iter is incremented.
					}
				}
				else if (iter->addr + iter->size > target_addr + target_size && !flag_inclusion (target_flags, iter->flags) ) {
					iter->size = iter->size - target_size;
					iter->addr = target_addr + target_size;
					insert_t.addr = target_addr;
					insert_t.size = target_size;
					insert_t.flags = target_flags | iter->flags;
					wp.insert(iter, insert_t);
					return;
				}
				else {
					insert_t.addr = iter->addr;
					insert_t.size = iter->size;
					insert_t.flags = iter->flags | target_flags;
					iter= wp.erase(iter);
				}
			}
			else {//Otherwise, we will need to consider splitting
				if (flag_inclusion (target_flags, iter->flags) ) {//if the flag is included
					//then just mark the node as "to be inserted"
					insert_t.addr = iter->addr;
					insert_t.size = iter->size;
					insert_t.flags = iter->flags;
					iter = wp.erase(iter);//!!As this watchpoint node is erased, iter automatically increments by 1.
				}
				else {//if the flag is not included, we then need to split the watchpoint
					if (iter->addr + iter->size > target_addr + target_size) {
						insert_t.size = iter->addr + iter->size - target_addr - target_size;
						insert_t.addr = target_addr + target_size;
						insert_t.flags = iter->flags;
						iter->size = target_addr - iter->addr;//split the watchpoint by modifying iter's length
						iter++;
						iter = wp.insert(iter, insert_t);
						insert_t.addr = target_addr;
						insert_t.size = target_size;
						insert_t.flags = target_flags | iter->flags;
						iter = wp.insert(iter, insert_t);
						return;
					}
					insert_t.size = iter->addr + iter->size - target_addr;
					insert_t.addr = target_addr;
					insert_t.flags = target_flags | iter->flags;
					iter->size = target_addr - iter->addr;//split the watchpoint by modifying iter's length
					iter++;//increment to the next node
				}
			}
		}
		else {//The target_addr is not covered at all by the iterator.
			if (iter != wp.begin()) {//Check if the watchpoint to be added should be merged with an wp before it.
				start_iter = iter - 1;
				if (start_iter->addr + start_iter->size == target_addr && start_iter->flags == target_flags) {
					insert_t.addr = start_iter->addr;
					if (iter->addr < target_addr + target_size)
						insert_t.size = iter->addr - insert_t.addr;
					else
						insert_t.size = target_size + target_addr - insert_t.addr;
					insert_t.flags = target_flags;
					iter = wp.erase(start_iter);
				}
				else {
					insert_t.addr = target_addr;
					if (iter->addr < target_addr + target_size)
						insert_t.size = iter->addr - target_addr;
					else
						insert_t.size = target_size;
					insert_t.flags = target_flags;
				}
			}
			else {
				insert_t.addr = target_addr;
				if (iter->addr < target_addr + target_size)
					insert_t.size = iter->addr - target_addr;
				else
					insert_t.size = target_size;
				insert_t.flags = target_flags;
			}
		}
		
		
		//Iterating part
		while (iter != wp.end() && iter->addr + iter->size <= target_addr + target_size) {
			if (iter->addr != insert_t.addr + insert_t.size) {//If there is some blank between the two nodes.
				if (insert_t.flags == target_flags)//if the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = iter->addr - insert_t.addr;
				else {//if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
					iter = wp.insert(iter, insert_t);
					iter++;
					insert_t.addr = insert_t.addr + insert_t.size;
					insert_t.size = iter->addr - insert_t.addr;
					insert_t.flags = target_flags;
				}
			}
			if (insert_t.flags == (iter->flags | target_flags) )//then we will need to merge the two node, by just enlarging the insert_t size
				insert_t.size = insert_t.size + iter->size;
			else {//if not, then we will first insert insert_t into wp, and update insert_t.
				iter = wp.insert(iter, insert_t);
				iter++;
				insert_t.addr = iter->addr;
				insert_t.size = iter->size;
				insert_t.flags = iter->flags | target_flags;
			}
			iter = wp.erase(iter);//anyway, we will need to delete the wp we walked through, since we will add them back in the future(Which is also in this code).
		}
		
		//Ending part
		if (iter == wp.end() || !addr_covered( (target_addr + target_size), (*iter) ) ) {//If it's the end of the deque or the end+1 address is not covered by any wp
			//Then we simply add the watchpoint on it.
			if (target_addr + target_size > insert_t.addr + insert_t.size) {//If there is still some blank between insert_t and the end of the adding address.
				if (insert_t.flags == target_flags) {//if the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = target_addr + target_size - insert_t.addr;
				}
				else {//if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
					if (iter == wp.end() ) {
						wp.push_back(insert_t);
						iter = wp.end();
					}
					else {
						iter = wp.insert(iter, insert_t);
						iter++;
					}
					insert_t.addr = insert_t.addr + insert_t.size;
					insert_t.size = target_addr + target_size - insert_t.addr;
					insert_t.flags = target_flags;
				}
			}
			//As this is the end of this adding procedure, we will fill up this blank.
			if (iter == wp.end() ) {
				wp.push_back(insert_t);
				iter = wp.end();
			}
			else {
				iter = wp.insert(iter, insert_t);
				iter++;
			}
		}
		else {//Then the end+1 address is covered by some wp. Then we'll need to test whether the flags are the same.
			//First check if there is any blank between the two nodes.
			if (iter->addr != insert_t.addr + insert_t.size) {//If there is some blank between the two nodes.
				if (insert_t.flags == target_flags)//if the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = iter->addr - insert_t.addr;
				else {//if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
					iter = wp.insert(iter, insert_t);
					iter++;
					insert_t.addr = insert_t.addr + insert_t.size;
					insert_t.size = iter->addr - insert_t.addr;
					insert_t.flags = target_flags;
				}
			}
			//If the flag of insert_t and the up-comming node are the same, then we definetly merge the two nodes
			if (insert_t.flags == iter->flags) {
				insert_t.size = insert_t.size + iter->size;
				iter = wp.insert(iter, insert_t);
				iter++;
				wp.erase(iter);
			}
			//If they aren't the same. We then need to merge part of them(maybe). And split the rest.
			else if (iter->addr < target_addr + target_size) {//If the ending address is not covered, no need to split. (Merging is done by above)
				if (flag_inclusion (target_flags, iter->flags) ) {//if the target_flag is included by iter, then we just left it as before
					iter = wp.insert(iter, insert_t);
				}
				else {
					if (insert_t.flags == (iter->flags | target_flags) ) {//Check the merge condition.
						insert_t.size = target_addr + target_size - insert_t.addr;
						iter = wp.insert(iter, insert_t);
						iter++;
					}
					else {
						
						//If we can't merge, we need to split the iter node
						iter = wp.insert(iter, insert_t);
						iter++;
						insert_t.addr = insert_t.addr + insert_t.size;
						insert_t.size = target_addr + target_size - insert_t.addr;
						insert_t.flags = iter->flags | target_flags;
						iter = wp.insert(iter, insert_t);
						iter++;
					}
					//splitting(modify the node which covers the ending address of the range)
					iter->size = iter->addr + iter->size - (target_addr + target_size);
					iter->addr = target_addr + target_size;
				}
			}
			else {
				iter = wp.insert(iter, insert_t);
				iter++;
			}
		}
		return;
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	void WatchPoint<ADDRESS, COUNT, FLAGS>::rm_watchpoint (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		if (target_size == 0)
			return;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator previous_iter;
		//starting part
		iter = search_address(target_addr, wp);
		if (iter == wp.end() )
			return;
		if (addr_covered(target_addr - 1, (*iter) ) ) {
			if (target_addr + target_size < iter->addr + iter->size) {
				if (target_flags & iter->flags) {
					watchpoint_t<ADDRESS, FLAGS> insert_t = {iter->addr, target_addr - iter->addr, iter->flags};
					iter->size = iter->addr + iter->size - target_addr - target_size;
					iter->addr = target_addr + target_size;
					iter = wp.insert(iter, insert_t);
					insert_t.flags = iter->flags & (~target_flags);
					if (insert_t.flags) {
						insert_t.addr = target_addr;
						insert_t.size = target_size;
						iter++;
						wp.insert(iter, insert_t);
					}
				}
				return;
			}
			if (target_flags & iter->flags) {
				watchpoint_t<ADDRESS, FLAGS> insert_t = {target_addr, iter->addr + iter->size - target_addr, iter->flags & (~target_flags)};
				iter->size = target_addr - iter->addr;
				iter++;
				if (insert_t.flags) {
					iter = wp.insert(iter, insert_t);
					iter++;
				}
			}
		}
		else if (target_addr + target_size < iter->addr + iter->size) {
			if (iter->flags & target_flags) {
				watchpoint_t<ADDRESS, FLAGS> insert_t = {iter->addr, target_addr + target_size - iter->addr, iter->flags & (~target_flags)};
				iter->size = iter->addr + iter->size - target_addr - target_size;
				iter->addr = target_addr + target_size;
				if (insert_t.flags) {
					previous_iter = iter - 1;
					if (previous_iter->addr + previous_iter->size == insert_t.addr && previous_iter->flags == insert_t.flags)
						previous_iter->size += insert_t.size;
					else {
						iter = wp.insert(iter, insert_t);
						iter++;
					}
				}
			}
			return;
		}

		//iterating part		
		while (iter != wp.end() && iter->addr + iter->size < target_addr + target_size) {
			if (iter->flags & target_flags) {
				iter->flags = iter->flags & (~target_flags);
				if (iter->flags) {
					previous_iter = iter - 1;
					if (previous_iter->addr + previous_iter->size == iter->addr && previous_iter->flags == iter->flags) {
						previous_iter->size += iter->size;
						iter = wp.erase(iter);
					}
					else
						iter++;
				}
				else
					iter = wp.erase(iter);
			}
			else
				iter++;
		iter++;
		}
		
		//ending part
		if (iter != wp.end() && addr_covered( (target_addr + target_size - 1), (*iter) ) && iter->flags & target_flags) {
			if (target_addr + target_size == iter->addr + iter->size) {
				iter->flags = iter->flags & (~target_flags);
				if (iter->flags) {
					previous_iter = iter - 1;
					if (previous_iter->addr + previous_iter->size == iter->addr && previous_iter->flags == iter->flags) {
						previous_iter->size += iter->size;
				 		iter = wp.erase(iter);
				 	}
				}
				else
					wp.erase(iter);
			}
			else {
				watchpoint_t<ADDRESS, FLAGS> insert_t = {iter->addr , target_addr + target_size - iter->addr, iter->flags & (~target_flags)};
				if (insert_t.flags) {
					previous_iter = iter - 1;
					if (previous_iter->addr + previous_iter->size == insert_t.addr && previous_iter->flags == insert_t.flags)
						previous_iter->size += insert_t.size;
					else {
						iter = wp.insert(iter, insert_t);
						iter++;
					}
				}
				iter->size = iter->addr + iter->size - target_addr - target_size;
				iter->addr = target_addr + target_size;
			}
		}
		previous_iter = iter - 1;
		if (iter != wp.end() && previous_iter->addr + previous_iter->size == iter->addr && previous_iter->flags == iter->flags) {
 			previous_iter->size += iter->size;
 			iter = wp.erase(iter);
 		}
		return;
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	void WatchPoint<ADDRESS, COUNT, FLAGS>::rm_watch (ADDRESS target_addr, ADDRESS target_size) {
		rm_watchpoint (target_addr, target_size, (WA_READ | WA_WRITE) );
		return;
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	void WatchPoint<ADDRESS, COUNT, FLAGS>::rm_read (ADDRESS target_addr, ADDRESS target_size) {
		rm_watchpoint (target_addr, target_size, WA_READ);
		return;
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	void WatchPoint<ADDRESS, COUNT, FLAGS>::rm_write (ADDRESS target_addr, ADDRESS target_size) {
		rm_watchpoint (target_addr, target_size, WA_WRITE);
		return;
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	COUNT WatchPoint<ADDRESS, COUNT, FLAGS>::general_fault (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		if (target_size == 0)
			return 0;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		iter = search_address(target_addr, wp);
		if (iter == wp.end() )
			return 0;
		COUNT fault_num = 0;
		while (iter->addr < target_addr + target_size) {
			if (iter->flags & target_flags)
				fault_num++;
			iter++;
		}
		return fault_num;
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	COUNT WatchPoint<ADDRESS, COUNT, FLAGS>::watch_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, (WA_READ | WA_WRITE) ) );
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	COUNT WatchPoint<ADDRESS, COUNT, FLAGS>::read_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, WA_READ) );
	}
	
	template <class ADDRESS, class COUNT, class FLAGS>
	COUNT WatchPoint<ADDRESS, COUNT, FLAGS>::write_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, WA_WRITE) );
	}
}

int main() {
	using namespace Hongyi_WatchPoint;
	int target_addr;
	int target_size;
	int target_flags;
	WatchPoint<int, int, int> watch;
	unsigned int i;

	//Adding the front wp
	target_addr = 15;
	target_size = 5;
	target_flags = WA_READ | WA_WRITE;
	watch.add_watchpoint (target_addr, target_size, target_flags);
	
	
	//Adding the end wp
	target_addr = 20;
	target_size = 5;
	target_flags = WA_WRITE;
	watch.add_watchpoint (target_addr, target_size, target_flags);
	
	target_addr = 26;
	target_size = 5;
	target_flags = WA_WRITE;
	watch.add_watchpoint (target_addr, target_size, target_flags);
	
	target_addr = 40;
	target_size = 5;
	target_flags = WA_READ;
	watch.add_watchpoint (target_addr, target_size, target_flags);
	cout << endl << "I've added the front and end watch points" << endl;

	watch.watch_print();

	target_addr = 15;
	target_size = 10;
	target_flags = WA_WRITE;
	int num = watch.general_fault (target_addr, target_size, target_flags);

	cout << endl << endl << "**How many wp from " << target_addr << " with size " << target_size << " fall into flags " << target_flags << ":" << endl;
	cout << num << endl;

	return 0;
}