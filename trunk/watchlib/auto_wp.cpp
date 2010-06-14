
//#define	WA_EXEC			4
//#define	WA_TRAPAFTER	8

#include <vector>
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

		void add_watchpoint (int target_addr, int target_size, int target_flags);
		void add_read_wp (int target_addr, int target_size);
		void add_write_wp (int target_addr, int target_size);
		
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
		for (iter = wp.begin(); iter != wp.end(); iter++) {
			if(iter->addr > target_addr || iter->addr + iter->size > target_addr)
				break;
		}
		return iter;
	}	
	
	bool addr_covered (int target_addr, const watchpoint_t& node) {
		return (target_addr >= node.addr && target_addr < node.addr + node.size);
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
		
//		cout << "**Now iter->addr is ";
//		if (iter == wp.end() )
//			cout << "NULL" << endl;
//		else
//			cout << iter->addr << endl;
		
		if (iter == wp.end() ) {
//			cout << "**Oh my god, it can't be !!" << endl;
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
			cout << "**If it enters here, then there must be a bug!" << endl;
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
						if (iter->addr + iter->size > target_addr + target_size && ( (target_flags ^ iter->flags) & iter->flags) != (target_flags ^ iter->flags) ) {
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
				else if (iter->addr + iter->size > target_addr + target_size && ( (target_flags ^ iter->flags) & iter->flags) != (target_flags ^ iter->flags) ) {
					cout << "****Oh please, it can't enter's here!!" << endl;
					iter->size = iter->size - target_size;
					iter->addr = target_addr + target_size;
					insert_t.addr = target_addr;
					insert_t.size = target_size;
					insert_t.flags = target_flags | iter->flags;
					wp.insert(iter, insert_t);
					return;
				}
				else {
					cout << "****If it's here then it's right!!" << endl;
					insert_t.addr = iter->addr;
					insert_t.size = iter->size;
					insert_t.flags = iter->flags | target_flags;
					iter= wp.erase(iter);
				}
			}
			else {//Otherwise, we will need to consider splitting
				if ( ( (target_flags ^ iter->flags) & iter->flags) == (target_flags ^ iter->flags) ) {//if the flag is included
					cout << "****Com'on, is it here!?!" << endl;
					//then just mark the node as "to be inserted"
					insert_t.addr = iter->addr;
					insert_t.size = iter->size;
					insert_t.flags = iter->flags;
					iter = wp.erase(iter);//!!As this watchpoint node is erased, iter automatically increments by 1.
				}
				else {//if the flag is not included, we then need to split the watchpoint
					cout << "OMG, it shouldn't be here!" << endl;
					if (iter->addr + iter->size > target_addr + target_size) {
						cout << "****Right instruction flow" << endl;
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
//			cout << "**Has it ever been here?" << endl;
			if (iter != wp.begin()) {//Check if the watchpoint to be added should be merged with an wp before it.
//				cout << "**No iter is wp.begin()!" << endl;
				start_iter = iter - 1;
				if (start_iter->addr + start_iter->size == target_addr && start_iter->flags == target_flags) {
					cout << "Has it entered here?" << endl;
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
//					iter++;//increment to the next node
				}
			}
			else {
//				cout << "**So, we will have nothing to do." << endl;	
				insert_t.addr = target_addr;
				if (iter->addr < target_addr + target_size)
					insert_t.size = iter->addr - target_addr;
				else
					insert_t.size = target_size;
				insert_t.flags = target_flags;
//				iter++;//increment to the next node
			}
		}
		
//		cout << "What is iter now?" <<endl;
//		if (iter == wp.end() )
//			cout << "iter was now pointing to NULL" << endl;
//		else
//			cout << "iter->addr = " << iter->addr << endl;
		
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
		
		if (iter == wp.end() )
			cout << "!!There should at least be one time" << endl;
		
		//Ending part
		if (iter == wp.end() || !addr_covered( (target_addr + target_size), (*iter) ) ) {//If it's the end of the vector or the end+1 address is not covered by any wp
			//Then we simply add the watchpoint on it.
//			cout << "The instruction flow is now in here and ready to insert the insert_t.0" << endl;
			if (target_addr + target_size > insert_t.addr + insert_t.size) {//If there is still some blank between insert_t and the end of the adding address.
				cout << "**if it enter's here, then 我败了" << endl;
				if (insert_t.flags == target_flags) {//if the insert node's flag matches the output node, then we just enlarge the insert node.
//					cout << "**Please, let me be right for 1 time!" << endl;
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
//			cout << "The instruction flow is now in here and ready to insert the insert_t.1" << endl;
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
//				cout << "The ending flags are equal and I'm going to merge them together!" <<endl;
//				cout << "Ok. Then let's see what is insert node before merge." << endl;
//				cout << "Insert_t.addr = " << insert_t.addr << endl;
//				cout << "Insert_t.size = " << insert_t.size << endl;
//				cout << "Insert_t.flags = " << insert_t.flags << endl;

//				cout << "Iter's address before insersion: " << iter->addr << endl;
				
				insert_t.size = insert_t.size + iter->size;
				
				iter = wp.insert(iter, insert_t);
				iter++;

//				cout << "Print after insersion" << endl;
//				watch_print();
				wp.erase(iter);
//				cout << "Print after erase" << endl;
//				watch_print();
			}
			//If they aren't the same. We then need to merge part of them(maybe). And split the rest.
			else if (iter->addr < target_addr + target_size) {//If the ending address is not covered, no need to split. (Merging is done by above)
				if ( ( (target_flags ^ iter->flags) & iter->flags) == (target_flags ^ iter->flags) ) {//if the target_flag is included by iter, then we just left it as before
					cout << "Ok let's see what inert_t is " << endl;
					cout << "Insert_t.addr = " << insert_t.addr << endl;
					cout << "Insert_t.size = " << insert_t.size << endl;
					cout << "Insert_t.flags = " << insert_t.flags << endl;
					cout << "And iter->addr is " << iter->addr << endl;
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
						cout << "It should be here !" << endl;
						cout << "Insert's current flags is: " << insert_t.flags << endl;
						iter = wp.insert(iter, insert_t);
						cout << "** Testing print **" << endl;
						watch_print();
						cout << "** Testing end **" << endl;
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

	target_addr = 20;
	target_size = 5;
	target_flags = WA_READ;
	watch.add_watchpoint (target_addr, target_size, target_flags);

	watch.watch_print();

	return 0;
}
