#ifndef VECTOR_H
#include <vector>
#define VECTOR_H
#endif

//#include "auto_wp.h"
#include <iostream>

namespace {
	#define	WA_READ			1
	#define	WA_WRITE		2
	
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

		
		void add_read_wp (int target_addr, int target_size);
		void add_write_wp (int target_addr, int target_size);
		
		void rm_watchpoint (int target_addr, int target_size, int target_flags);
		
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


namespace {
	vector<watchpoint_t>::iterator search_address(int target_addr, vector<watchpoint_t>& wp) {
		vector<watchpoint_t>::iterator iter;
		for (iter = wp.begin(); iter != wp.end(); iter++) {
			if(iter->addr > target_addr || iter->addr + iter->size > target_addr)
				break;
		}
		return iter;
	}	
	
	bool addr_covered (int target_addr, const watchpoint_t& node) {
		return (target_addr >= node.addr && target_addr < node.addr + node.size);
	}
	
	bool flag_inclusion (int target_flags, int container_flags) {
		return ( ( (target_flags ^ container_flags) & container_flags) == (target_flags ^ container_flags) );
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
		cout << "There are " << wp.size() << " watchpoints" << endl;
		for (i = 0; i < wp.size(); i++) {
			cout << "This is the " << i << "th watchpoint." <<endl;
			cout << "The watchpoint is at " << wp[i].addr << endl;
			cout << "The watchpoint has a size of " << wp[i].size << endl;
			if (wp[i].flags & WA_READ)
				cout << "||READ||" << endl;
			if (wp[i].flags & WA_WRITE)
				cout << "||WRITE||" << endl;
		}
	}
	
	void WatchPoint::add_read_wp (int target_addr, int target_size) {
		add_watchpoint (target_addr, target_size, WA_READ);
		return;
	}
	
	void WatchPoint::add_write_wp (int target_addr, int target_size) {
		add_watchpoint (target_addr, target_size, WA_WRITE);
		return;
	}
	
	void WatchPoint::add_watchpoint(int target_addr, int target_size, int target_flags) {
		if (target_size == 0)
			return;
		watchpoint_t insert_t = {0, 0, 0};
		vector<watchpoint_t>::iterator iter;
		vector<watchpoint_t>::iterator start_iter;//This one is used only for merging the front wp nodes.
		iter = search_address(target_addr, wp);
		
		if (iter == wp.end() ) {
			if (iter != wp.begin() ) {//We'll need to check if there is some wp ahead of iter and if there is we need to decide whether merge the two.
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
		if (iter == wp.end() || !addr_covered( (target_addr + target_size), (*iter) ) ) {//If it's the end of the vector or the end+1 address is not covered by any wp
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
	
	void WatchPoint::rm_watchpoint (int target_addr, int target_size, int target_flags) {
		if (target_size == 0)
			return;
		vector<watchpoint_t>::iterator iter;
		vector<watchpoint_t>::iterator previous_iter;
		//starting part
		iter = search_address(target_addr, wp);
		if (iter == wp.end() )
			return;
		if (addr_covered(target_addr - 1, (*iter) ) ) {
			if (target_addr + target_size < iter->addr + iter->size) {
				if (target_flags & iter->flags) {
					watchpoint_t insert_t = {iter->addr, target_addr - iter->addr, iter->flags};
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
				watchpoint_t insert_t = {target_addr, iter->addr + iter->size - target_addr, iter->flags & (~target_flags)};
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
				watchpoint_t insert_t = {iter->addr, target_addr + target_size - iter->addr, iter->flags & (~target_flags)};
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
				 		if (iter != wp.end() && previous_iter->addr + previous_iter->size == iter->addr && previous_iter->flags == iter->flags) {
				 			previous_iter->size += iter->size;
				 			wp.erase(iter);
				 		}
				 	}
				}
				else
					wp.erase(iter);
			}
			else {
				watchpoint_t insert_t = {iter->addr , target_addr + target_size - iter->addr, iter->flags & (~target_flags)};
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
		return;
	}
	
	int WatchPoint::general_fault (int target_addr, int target_size, int target_flags) {
		if (target_size == 0)
			return 0;
		vector<watchpoint_t>::iterator iter;
		iter = search_address(target_addr, wp);
		if (iter == wp.end() )
			return 0;
		int fault_num = 0;
		while (iter->addr < target_addr + target_size) {
			if (iter->flags & target_flags)
				fault_num++;
			iter++;
		}
		return fault_num;
	}
	
	int WatchPoint::watch_fault(int target_addr, int target_size) {
		return (general_fault (target_addr, target_size, (WA_READ | WA_WRITE) ) );
	}
	
	int WatchPoint::read_fault(int target_addr, int target_size) {
		return (general_fault (target_addr, target_size, WA_READ) );
	}
	
	int WatchPoint::write_fault(int target_addr, int target_size) {
		return (general_fault (target_addr, target_size, WA_WRITE) );
	}
}

int main() {
	using namespace Hongyi_WatchPoint;
	int target_addr;
	int target_size;
	int target_flags;
	WatchPoint watch;
	int i;
	
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
	
	target_addr = 25;
	target_size = 5;
	target_flags = WA_READ | WA_WRITE;
	watch.add_watchpoint (target_addr, target_size, target_flags);
	
	target_addr = 40;
	target_size = 5;
	target_flags = WA_WRITE;
	watch.add_watchpoint (target_addr, target_size, target_flags);
	cout << endl << "I've added the front and end watch points" << endl;

	watch.watch_print();

	target_addr = 19;
	target_size = 1;
	target_flags = WA_READ;
	watch.rm_watchpoint (target_addr, target_size, target_flags);

	cout << "**I've removed the watchpoint**" << endl;
	watch.watch_print();
//	cout << endl << endl << "**How many wp from " << target_addr << " with size " << target_size << " fall into flags " << target_flags << ":" << endl;
//	cout << num << endl;

	return 0;
}


