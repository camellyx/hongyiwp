//////////////////////////////////////////////////////
//													//
//		This is the headder file of the automatic	//
//		watchpoint implementation. It includes		//
//		automatic watchpoint adding and removing	//
//													//
//													//
//////////////////////////////////////////////////////
#ifndef WP_H
#include <deque>
#include <iostream>
#define WP_H
#endif

using std::cout;
using std::endl;
using std::deque;

namespace Hongyi_WatchPoint {
	#define	WA_READ			1
	#define	WA_WRITE		2
	
	template<class ADDRESS, class FLAGS>
	struct watchpoint_t {
		ADDRESS addr;
		ADDRESS size;
		FLAGS flags;
	};
	
	struct trie_data_t {
		unsigned int top_hit;
		unsigned int mid_hit;
		unsigned int bot_hit;
		
		unsigned int top_change;
		unsigned int mid_change;
		unsigned int bot_change;
		
		unsigned int top_break;
		unsigned int mid_break;
		
		const trie_data_t operator+(const trie_data_t &other) const;
	};
	
	const trie_data_t trie_data_t::operator+(const trie_data_t &other) const {
		trie_data_t result = *this;
		result.top_hit += other.top_hit;
		result.mid_hit += other.mid_hit;
		result.bot_hit += other.bot_hit;
		result.top_change += other.top_change;
		result.mid_change += other.mid_change;
		result.bot_change += other.mid_change;
		result.top_break += other.top_break;
		result.mid_break += other.mid_break;
		return result;
	}
	
	enum PAGE_HIT {TOP, MID, BOT};
	
//	deque<watchpoint_t>::iterator search_address(ADDRESS target_addr, deque<watchpoint_t>& wp);
	
	template<class ADDRESS, class FLAGS>
	class WatchPoint {
	public:
		//Constructors
		WatchPoint();
		WatchPoint(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		WatchPoint(const WatchPoint& parameter);
		
		void	clear	();

		void	add_watch_wp	(ADDRESS target_addr, ADDRESS target_size);
		void	add_read_wp		(ADDRESS target_addr, ADDRESS target_size);
		void	add_write_wp	(ADDRESS target_addr, ADDRESS target_size);
		
		void	rm_watch	(ADDRESS target_addr, ADDRESS target_size);
		void	rm_read		(ADDRESS target_addr, ADDRESS target_size);
		void	rm_write	(ADDRESS target_addr, ADDRESS target_size);
		
		bool	watch_fault	(ADDRESS target_addr, ADDRESS target_size);
		//return: The number of how many watchpoints it touches within the range, regardless what kind of flags the watchpoint has.
		bool	read_fault	(ADDRESS target_addr, ADDRESS target_size);
		//return: The number of how many *read* watchpoints it touches within the range.
		bool	write_fault	(ADDRESS target_addr, ADDRESS target_size);
		//return: The number of how many *write* watchpoints it touches within the range.
		
		trie_data_t	get_trie_data ();
		//return: The trie data.
		
		void	watch_print();
		
		//all below are private
		void rm_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		void add_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		bool general_fault	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags,  unsigned int& top_page, unsigned int& mid_page, unsigned int& bot_page);
		PAGE_HIT page_level	(ADDRESS target_addr, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter);
		void page_break		(PAGE_HIT& before, PAGE_HIT& after);
		
		deque< watchpoint_t<ADDRESS, FLAGS> > wp;
		trie_data_t trie;
	private:
/*
		void rm_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		void add_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		COUNT general_fault (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		deque< watchpoint_t<ADDRESS, FLAGS> > wp;
*/
	};
}

using Hongyi_WatchPoint::watchpoint_t;

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
		
		if (end->addr + end->size <= target_addr)
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
	
	template <class ADDRESS, class FLAGS>
	WatchPoint<ADDRESS, FLAGS>::WatchPoint() {
		trie.top_hit = 0;
		trie.mid_hit = 0;
		trie.bot_hit = 0;
		
		trie.top_change = 0;
		trie.mid_change = 0;
		trie.bot_change = 0;
		
		trie.top_break = 0;
		trie.mid_break = 0;
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	WatchPoint<ADDRESS, FLAGS>::WatchPoint(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		watchpoint_t<ADDRESS, FLAGS> temp = {target_addr, target_size, target_flags};
		wp.push_back(temp);
		trie.top_hit = 0;
		trie.mid_hit = 0;
		trie.bot_hit = 0;
		
		trie.top_change = 0;
		trie.mid_change = 0;
		trie.bot_change = 0;
		
		trie.top_break = 0;
		trie.mid_break = 0;
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	WatchPoint<ADDRESS, FLAGS>::WatchPoint(const WatchPoint& parameter) {
		wp.resize((int)parameter.wp.size());
		int i;
		for (i = 0; i < parameter.wp.size(); i++) {
			wp[i] = parameter.wp[i];
		}
		trie.top_hit = parameter.trie.top_hit;
		trie.mid_hit = parameter.trie.mid_hit;
		trie.bot_hit = parameter.trie.bot_hit;
		
		trie.top_change = trie.top_change;
		trie.mid_change = trie.mid_change;
		trie.bot_change = trie.bot_change;
		
		trie.top_break = trie.top_break;
		trie.mid_break = trie.mid_break;
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::clear() {
		wp.clear();
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::watch_print() {
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
		return;
	}

	template <class ADDRESS, class FLAGS>
	PAGE_HIT WatchPoint<ADDRESS, FLAGS>::page_level (ADDRESS addr, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter) {
		PAGE_HIT hit = TOP;
		if (iter != wp.end() && addr_covered(addr, *iter) ) {
			if (!addr_covered( (addr & ~(4095) ) , *iter) || !addr_covered( ( (addr + 4096) & ~(4095) ) , *iter) ) {
				hit = BOT;
				return hit;
			}
			else if (!addr_covered( (addr & ~(4194303) ) , *iter) || !addr_covered( ( (addr + 4194304) & ~(4194303) ) , *iter) )
				hit = MID;
		}
		else {
			if (iter != wp.end() && addr > (iter->addr & ~(4095) ) ) {
				hit = BOT;
				return hit;
			}
			else if (iter != wp.end() && addr > (iter->addr & ~(4194303) ) )
				hit = MID;
			
			if (iter != wp.begin() ) {
				iter--;
				if (iter->addr + iter->size > (addr & ~(4095) ) ) {
					hit = BOT;
					return hit;
				}
				if (iter->addr + iter->size > (addr & ~(4194303) ) )
					hit = MID;
			}
		}
		return hit;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::page_break (PAGE_HIT& before, PAGE_HIT& after) {
		if (before == TOP && after != TOP)
			trie.top_break++;
		else if (before == MID && after == BOT)
			trie.mid_break++;
		return;
	}
		
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::add_watch_wp (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, beg_iter);
		
		add_watchpoint (target_addr, target_size, WA_READ | WA_WRITE);
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, beg_iter);
		
		page_break (begin_hit_before, begin_hit_after);
		page_break (end_hit_before, end_hit_after);
		
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::add_read_wp (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, beg_iter);
		
		add_watchpoint (target_addr, target_size, WA_READ);
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, beg_iter);
		
		page_break (begin_hit_before, begin_hit_after);
		page_break (end_hit_before, end_hit_after);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::add_write_wp (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, beg_iter);
		
		add_watchpoint (target_addr, target_size, WA_WRITE);
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, beg_iter);
		
		page_break (begin_hit_before, begin_hit_after);
		page_break (end_hit_before, end_hit_after);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::add_watchpoint(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
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
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::rm_watchpoint (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
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
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::rm_watch (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, beg_iter);
		
		rm_watchpoint (target_addr, target_size, (WA_READ | WA_WRITE) );
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, beg_iter);
		
		page_break (begin_hit_before, begin_hit_after);
		page_break (end_hit_before, end_hit_after);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::rm_read (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, beg_iter);
		
		rm_watchpoint (target_addr, target_size, WA_READ);
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, beg_iter);
		
		page_break (begin_hit_before, begin_hit_after);
		page_break (end_hit_before, end_hit_after);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::rm_write (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, beg_iter);
		
		rm_watchpoint (target_addr, target_size, WA_WRITE);
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, beg_iter);
		
		page_break (begin_hit_before, begin_hit_after);
		page_break (end_hit_before, end_hit_after);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	bool WatchPoint<ADDRESS, FLAGS>::general_fault (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags, unsigned int& top_page, unsigned int& mid_page, unsigned int& bot_page) {
		if (target_size == 0)
			return false;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		iter = search_address(target_addr, wp);
		if (iter == wp.end() ) {
			if (iter != wp.begin() ) {
				iter--;
				if (iter->addr + iter->size > (target_addr & ~(4095) ) ) {
					cout << "bot_modify -1" << endl;
					bot_page++;
				}
				else if (iter->addr + iter->size > (target_addr & ~(4194303) ) ) {
					cout << "mid_modify -1" << endl;
					mid_page++;
				}
				else
					top_page++;
			}
			else
				top_page++;
			return false;
		}
		
		bool wp_fault = false;
		bool mid_level = false;
		bool bot_level = false;
		if(!addr_covered(target_addr, *iter) && iter !=wp.begin() ) {
			iter--;
			if (iter->addr + iter->size > (target_addr & ~(4095) ) ) {
				cout << "bot_modify 0" << endl;
				bot_level = true;
			}
			else if (iter->addr + iter->size > (target_addr & ~(4194303) ) ) {
				cout << "mid_modify 0" << endl;
				mid_level = true;
			}
			iter++;
		}
		
		ADDRESS beg_addr;
		ADDRESS end_addr;
		
		while (iter != wp.end() && iter->addr < target_addr + target_size) {
			if (iter->flags & target_flags)
				wp_fault = true;
			
			if (iter->addr > target_addr || iter->addr > (target_addr & ~(4095) ) ) {
				cout << "begin_addr is the bot_page beginning" << endl;
				beg_addr = iter->addr;
			}
			else if (iter->addr > (target_addr & ~(4194303) ) ) {
				cout << "begin_addr is the mid_page beginning" << endl;
				beg_addr = target_addr & ~(4095);
			}
			else {
				cout << "begin_addr is the top_page beginning" << endl;
				beg_addr = target_addr & ~(4194303);
			}
				
			if ( (iter->addr + iter->size <= target_addr + target_size) || iter->addr + iter->size < ( (target_addr + target_size + 4095) & ~(4095) ) ) {
				cout << "end_addr is the bot_page ending" << endl;
				end_addr = iter->addr + iter->size;
			}
			else if ( iter->addr + iter->size < ( (target_addr + target_size + 4194303) & ~(4194303) ) ) {
				cout << "end_addr is the mid_page ending" << endl;
				end_addr = (target_addr + target_size + 4095) & ~(4095);
			}
			else {
				cout << "end_addr is the top_page ending" << endl;
				end_addr = (target_addr + target_size + 4194303) & ~(4194303);
			}
			
			cout << "begin_addr: " << beg_addr << endl;
			cout << "end_addr: " << end_addr << endl;
			if ( (beg_addr & 4095) || (end_addr & 4095) ) {

				cout << "bot_modify 1" << endl;
				bot_level = true;
			}
			else if ( (beg_addr & 4194303) || (end_addr & 4194303) ) {
				cout << "mid_modify 1" << endl;
				mid_level = true;
			}
			iter++;
		}
		
		if (iter != wp.end() && iter->addr < ( (target_addr + target_size + 4095) & ~(4095) ) ) {
			cout << "bot_modify 2" << endl;
			bot_level = true;
		}
		else if (iter != wp.end() && iter->addr < ( (target_addr + target_size + 4194303) & ~(4194303) ) ) {
			cout << "mid_modify 2" << endl;
			mid_level = true;
		}
		if (bot_level)
			bot_page++;
		else if (mid_level)
			mid_page++;
		else
			top_page++;
			
		return wp_fault;
	}
	
	template <class ADDRESS, class FLAGS>
	bool WatchPoint<ADDRESS, FLAGS>::watch_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_hit, trie.mid_hit, trie.bot_hit) );
	}
	
	template <class ADDRESS, class FLAGS>
	bool WatchPoint<ADDRESS, FLAGS>::read_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, WA_READ, trie.top_hit, trie.mid_hit, trie.bot_hit) );
	}
	
	template <class ADDRESS, class FLAGS>
	bool WatchPoint<ADDRESS, FLAGS>::write_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, WA_WRITE, trie.top_hit, trie.mid_hit, trie.bot_hit) );
	}
	
	template <class ADDRESS, class FLAGS>
	trie_data_t WatchPoint<ADDRESS, FLAGS>::get_trie_data () {
		return trie;
	}
}

