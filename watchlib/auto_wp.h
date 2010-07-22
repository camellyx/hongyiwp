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

#define RANGE_CACHE_SIZE	64

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

	template<class ADDRESS>
	struct range_t {
		ADDRESS	start_addr;
		ADDRESS	end_addr;
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
		trie_data_t();
	};

	struct range_data_t {
		unsigned int max_range_num;
		double avg_range_num;
		unsigned int cur_range_num;
		unsigned int changes;
		unsigned int hit;
		unsigned int miss;
		unsigned int kick;
	};
	
	trie_data_t::trie_data_t() {
		top_hit = 0;
		mid_hit = 0;
		bot_hit = 0;
		
		top_change = 0;
		mid_change = 0;
		bot_change = 0;
		
		top_break = 0;
		mid_break = 0;
	}
	
	const trie_data_t trie_data_t::operator+(const trie_data_t &other) const {
		trie_data_t result = *this;
		result.top_hit += other.top_hit;
		result.mid_hit += other.mid_hit;
		result.bot_hit += other.bot_hit;
		result.top_change += other.top_change;
		result.mid_change += other.mid_change;
		result.bot_change += other.bot_change;
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
		
		trie_data_t	get_trie_data();
		//return: The trie data.
		void	reset_trie();
		
		void	watch_print();
		
		//all below are private
		void rm_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		void add_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		bool general_fault	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags,  unsigned int& top_page, unsigned int& mid_page, unsigned int& bot_page);
		PAGE_HIT page_level	(ADDRESS target_addr, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter);
		void page_break		(PAGE_HIT& before, PAGE_HIT& after);
		
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator	Insert_wp	(const watchpoint_t<ADDRESS, FLAGS>& insert_t, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter);//This would call wp.insert() internally and forward the return value out. But meanwhile it would emulate range cache access
		void														Modify_wp	(const watchpoint_t<ADDRESS, FLAGS>& modify_t, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter);//This would change the value of iter's node. And emualte range cache_access
		void														Push_back_wp	(const watchpoint_t<ADDRESS, FLAGS>& insert_t);//This would call wp.push_back() internally and emulate range_cache access
		typename deque< watchpoint_t<ADDRESS, FLAGS> >::iterator	Erase_wp	(typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter);//This would call wp.erase() internally and forward the return value out. Meanwhile emulate range_cache access
		typename deque< range_t<ADDRESS> >::iterator				Range_load	(ADDRESS start_addr,	ADDRESS end_addr);//
		//Func:		Search within the range_cache, and automatically increment hits and misses.
		//return:	the iterator that points to the range_t that is [start_addr, end_addr]. If the searched range is not within cache it would be insert at the front(The most recently used).
		void														Range_cleanup();
		//Func:		This function would be called in Insert_wp, Modify_wp, Push_back_wp and Erase_wp. It would check if after these calling, is range_cache has overflow? If there are, then increment the kick number.
		
		
		deque< watchpoint_t<ADDRESS, FLAGS> >	wp;
		deque< range_t<ADDRESS> >	range_cache;
		
		range_data_t	range;
		trie_data_t		trie;
	private:
/*
		void rm_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		void add_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		COUNT general_fault (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		deque< watchpoint_t<ADDRESS, FLAGS> > wp;
*/
	};
	
	template <class ADDRESS, class FLAGS>
	class MEM_WatchPoint {
	public:
		//Constructors
		MEM_WatchPoint();
		MEM_WatchPoint(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		MEM_WatchPoint(const MEM_WatchPoint& parameter);
		
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
		
		void	DumpStart();
		//		would initialize the dump_iter;
		watchpoint_t<ADDRESS, FLAGS>	Dump();
		//		would return the watchpoint_t and increment the dump_iter
		bool	DumpEnd();
		//		return ture if dump_iter reaches the end of wp.
		
		void	watch_print();
		
		void rm_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		void add_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		bool general_fault (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		deque< watchpoint_t<ADDRESS, FLAGS> > wp;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator dump_iter;
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
		return (target_addr >= node.addr && target_addr <= node.addr + node.size - 1);
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
		
		if (beg != wp.end() && beg->size == 0)//This is special for size = 0;
			return wp.begin();
		
		if (end->addr + end->size - 1 < target_addr)
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
		
		//Below is for range.
		range.max_range_num = 1;
		range.avg_range_num = 1;
		range.cur_range_num = 1;
		range.changes = 1;
		range.hit = 0;
		range.miss = 0;
		range.kick = 0;
		//Also intiallize the range cache.
		range_t<ADDRESS>	insert_range = {0, -1};
		range_cache.push_back(insert_range);
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
		
		//Below is for range.
		range.max_range_num = 1;
		range.avg_range_num = 1;
		range.cur_range_num = 1;
		range.changes = 1;
		range.hit = 0;
		range.miss = 0;
		range.kick = 0;
		//Also intiallize the range cache.
		range_t<ADDRESS>	insert_range = {0, -1};
		range_cache.push_back(insert_range);
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
				if (iter->addr + iter->size - 1 >= (addr & ~(4095) ) ) {
					hit = BOT;
					return hit;
				}
				if (iter->addr + iter->size - 1 >= (addr & ~(4194303) ) )
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
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);
		
		add_watchpoint (target_addr, target_size, WA_READ | WA_WRITE);
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
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
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);
		
		add_watchpoint (target_addr, target_size, WA_READ);
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
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
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);
		
		add_watchpoint (target_addr, target_size, WA_WRITE);
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
		page_break (begin_hit_before, begin_hit_after);
		page_break (end_hit_before, end_hit_after);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::add_watchpoint(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		watchpoint_t<ADDRESS, FLAGS> insert_t = {0, 0, 0};
		watchpoint_t<ADDRESS, FLAGS> modify_t = {0, 0, 0};
		if (target_size == 0) {//This is special for size = 0. As it clears all the current watchpoints and add the whole system as watched.
			wp.clear();
			insert_t.flags = target_flags;
			range_cache.clear();
			range_t<ADDRESS> insert_range = {0, -1};
			range_cache.push_back(insert_range);
			Push_back_wp(insert_t);
			//wp.push_back(insert_t);
			return;
		}
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator start_iter;//This one is used only for merging the front wp nodes.
		iter = search_address(target_addr, wp);
		
		if (iter != wp.end() && iter->size == 0) {// This is special for size = 0. It would then check for flag inclusion and split it if necessary.
			if (!flag_inclusion (target_flags, iter->flags) ) {
				if (target_addr == 0) {
					insert_t.addr = target_addr + target_size;
					insert_t.size = 0 - insert_t.addr;
					insert_t.flags = iter->flags;
					modify_t.addr = iter->addr;
					modify_t.size = target_size;
					modify_t.flags = target_flags | iter->flags;
					Modify_wp(modify_t,iter);
					//iter->size = target_size;
					//iter->flags = target_flags | iter->flags;
					Push_back_wp(insert_t);
					//wp.push_back(insert_t);
				}
				else if (target_addr + target_size == 0) {
					insert_t.addr = target_addr;
					insert_t.size = target_size;
					insert_t.flags = iter->flags | target_flags;
					modify_t.addr =iter->addr;
					modify_t.size = target_addr;
					modify_t.flags = iter->flags;
					Modify_wp(modify_t, iter);
					//iter->size = target_addr;
					Push_back_wp(insert_t);
					//wp.push_back(insert_t);
				}
				else {
					insert_t.addr = 0;
					insert_t.size = target_addr;
					insert_t.flags = iter->flags;
					modify_t.addr = target_addr;
					modify_t.size = target_size;
					modify_t.flags = target_flags | iter->flags;
					Modify_wp(modify_t, iter);
					//iter->addr = target_addr;
					//iter->size = target_size;
					//iter->flags = target_flags | iter->flags;
					iter = Insert_wp(insert_t, iter);
					//iter = wp.insert(iter, insert_t);
					insert_t.addr = target_addr + target_size;
					insert_t.size = 0 - insert_t.addr;
					insert_t.flags = iter->flags;
					Push_back_wp(insert_t);
					//wp.push_back(insert_t);
				}
			}
			return;
		}
			
		if (iter == wp.end() ) {
			if (iter != wp.begin() ) {//We'll need to check if there is some wp ahead of iter and if there is then we need to decide whether merge the two.
				start_iter = iter - 1;
				if (start_iter->addr + start_iter->size == target_addr && start_iter->flags == target_flags) {//Merge condition.
					modify_t.addr = start_iter->addr;
					modify_t.size = start_iter->size + target_size;
					modify_t.flags = start_iter->flags;
					Modify_wp(modify_t, start_iter);
					//start_iter->size = start_iter->size + target_size;//Merge by enlarging the former wp node. It changes size
					return;
				}
			}
			insert_t.addr = target_addr;
			insert_t.size = target_size;
			insert_t.flags = target_flags;
			Push_back_wp(insert_t);
			//wp.push_back(insert_t);//A push back.
			return;
		}
		if (addr_covered (target_addr, (*iter) ) ) {
			if (iter->addr == target_addr) {//We will only need to consider merging if this is true.
				if (iter != wp.begin() ) {
					start_iter = iter - 1;
					if(start_iter->addr + start_iter->size == target_addr && start_iter->flags == (iter->flags | target_flags) ) {//We will have to merge the two node.
						insert_t.addr = start_iter->addr;
						insert_t.flags = start_iter->flags;
						if (iter->addr + iter->size - 1 > target_addr + target_size - 1 && !flag_inclusion (target_flags, iter->flags) ) {
							modify_t.addr = start_iter->addr;
							modify_t.size = start_iter->size + target_size;
							modify_t.flags = start_iter->flags;
							Modify_wp(modify_t, start_iter);
							//start_iter->size += target_size;//It changes size
							modify_t.addr = target_addr + target_size;
							modify_t.size = iter->size - target_size;
							modify_t.flags = iter->flags;
							Modify_wp(modify_t, iter);
							//iter->addr = target_addr + target_size;//It changes size
							//iter->size -= target_size;
							return;
						}
						insert_t.size = start_iter->size + iter->size;
						iter = Erase_wp(start_iter);
						//iter = wp.erase(start_iter);//Erase!
						iter = Erase_wp(iter);
						//iter = wp.erase(iter);//Erase!
					}
					else {
						insert_t.addr = iter->addr;
						insert_t.flags = iter->flags | target_flags;
						if (iter->addr + iter->size - 1 > target_addr + target_size - 1 && !flag_inclusion (target_flags, iter->flags) ) {
							modify_t.addr = target_addr + target_size;
							modify_t.size = iter->size - target_size;
							modify_t.flags = iter->flags;
							Modify_wp(modify_t, iter);
							//iter->size = iter->size - target_size;//It changes size
							//iter->addr = target_addr + target_size;
							insert_t.size = target_size;
							Insert_wp(insert_t, iter);
							//wp.insert(iter, insert_t);//An insert
							return;
						}
						insert_t.size = iter->size;
						iter = Erase_wp(iter);
						//iter = wp.erase(iter);//iter is incremented.//Erase!
					}
				}
				else if (iter->addr + iter->size - 1 > target_addr + target_size - 1 && !flag_inclusion (target_flags, iter->flags) ) {
					modify_t.addr = target_addr + target_size;
					modify_t.size = iter->size - target_size;
					modify_t.flags = iter->flags;
					Modify_wp(modify_t, iter);
					//iter->size = iter->size - target_size;//It changes size
					//iter->addr = target_addr + target_size;
					insert_t.addr = target_addr;
					insert_t.size = target_size;
					insert_t.flags = target_flags | iter->flags;
					Insert_wp(insert_t, iter);
					//wp.insert(iter, insert_t);//An insert
					return;
				}
				else {
					insert_t.addr = iter->addr;
					insert_t.size = iter->size;
					insert_t.flags = iter->flags | target_flags;
					iter = Erase_wp(iter);
					//iter= wp.erase(iter);//Erase!
				}
			}
			else {//Otherwise, we will need to consider splitting
				if (flag_inclusion (target_flags, iter->flags) ) {//if the flag is included
					//then just mark the node as "to be inserted"
					insert_t.addr = iter->addr;
					insert_t.size = iter->size;
					insert_t.flags = iter->flags;
					iter = Erase_wp(iter);
					//iter = wp.erase(iter);//!!As this watchpoint node is erased, iter automatically increments by 1.//Erase!
				}
				else {//if the flag is not included, we then need to split the watchpoint
					if (iter->addr + iter->size - 1 > target_addr + target_size - 1) {
						insert_t.size = iter->addr + iter->size - target_addr - target_size;
						insert_t.addr = target_addr + target_size;
						insert_t.flags = iter->flags;
						modify_t.addr = iter->addr;
						modify_t.size = target_addr - iter->addr;
						modify_t.flags = iter->flags;
						Modify_wp(modify_t, iter);
						//iter->size = target_addr - iter->addr;//split the watchpoint by modifying iter's length. It changes size.
						iter++;
						iter = Insert_wp(insert_t, iter);
						//iter = wp.insert(iter, insert_t);//1st insert
						insert_t.addr = target_addr;
						insert_t.size = target_size;
						insert_t.flags = target_flags | iter->flags;
						iter = Insert_wp(insert_t, iter);
						//iter = wp.insert(iter, insert_t);//2en insert
						return;
					}
					insert_t.size = iter->addr + iter->size - target_addr;
					insert_t.addr = target_addr;
					insert_t.flags = target_flags | iter->flags;
					modify_t.addr = iter->addr;
					modify_t.size = target_addr - iter->addr;
					modify_t.flags = iter->flags;
					Modify_wp(modify_t, iter);
					//iter->size = target_addr - iter->addr;//split the watchpoint by modifying iter's length. It changes size.
					iter++;//increment to the next node
				}
			}
		}
		else {//The target_addr is not covered at all by the iterator.
			if (iter != wp.begin()) {//Check if the watchpoint to be added should be merged with an wp before it.
				start_iter = iter - 1;
				if (start_iter->addr + start_iter->size == target_addr && start_iter->flags == target_flags) {
					insert_t.addr = start_iter->addr;
					if (iter->addr <= target_addr + target_size - 1)
						insert_t.size = iter->addr - insert_t.addr;
					else
						insert_t.size = target_size + target_addr - insert_t.addr;
					insert_t.flags = target_flags;
					iter = Erase_wp(start_iter);
					//iter = wp.erase(start_iter);//Erase!
				}
				else {
					insert_t.addr = target_addr;
					if (iter->addr <= target_addr + target_size - 1)
						insert_t.size = iter->addr - target_addr;
					else
						insert_t.size = target_size;
					insert_t.flags = target_flags;
				}
			}
			else {
				insert_t.addr = target_addr;
				if (iter->addr <= target_addr + target_size - 1)
					insert_t.size = iter->addr - target_addr;
				else
					insert_t.size = target_size;
				insert_t.flags = target_flags;
			}
		}
		
		
		//Iterating part
		while (iter != wp.end() && iter->addr + iter->size - 1 <= target_addr + target_size - 1) {
			if (iter->addr != insert_t.addr + insert_t.size) {//If there is some blank between the two nodes.
				if (insert_t.flags == target_flags)//if the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = iter->addr - insert_t.addr;
				else {//if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
					iter = Insert_wp(insert_t, iter);
					//iter = wp.insert(iter, insert_t);// An insert
					iter++;
					insert_t.addr = insert_t.addr + insert_t.size;
					insert_t.size = iter->addr - insert_t.addr;
					insert_t.flags = target_flags;
				}
			}
			if (insert_t.flags == (iter->flags | target_flags) )//then we will need to merge the two node, by just enlarging the insert_t size
				insert_t.size = insert_t.size + iter->size;
			else {//if not, then we will first insert insert_t into wp, and update insert_t.
				iter = Insert_wp(insert_t, iter);
				//iter = wp.insert(iter, insert_t);//An insert
				iter++;
				insert_t.addr = iter->addr;
				insert_t.size = iter->size;
				insert_t.flags = iter->flags | target_flags;
			}
			iter = Erase_wp(iter);
			//iter = wp.erase(iter);//anyway, we will need to delete the wp we walked through, since we will add them back in the future(Which is also in this code).//Erase!
		}
		
		//Ending part
		if (iter == wp.end() || !addr_covered( (target_addr + target_size), (*iter) ) ) {//If it's the end of the deque or the end+1 address is not covered by any wp
			//Then we simply add the watchpoint on it.
			if (target_addr + target_size - 1 > insert_t.addr + insert_t.size - 1) {//If there is still some blank between insert_t and the end of the adding address.
				if (insert_t.flags == target_flags) {//if the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = target_addr + target_size - insert_t.addr;
				}
				else {//if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
					if (iter == wp.end() ) {
						Push_back_wp(insert_t);
						//wp.push_back(insert_t);//a push back
						iter = wp.end();
					}
					else {
						iter = Insert_wp(insert_t, iter);
						//iter = wp.insert(iter, insert_t);//An insert
						iter++;
					}
					insert_t.addr = insert_t.addr + insert_t.size;
					insert_t.size = target_addr + target_size - insert_t.addr;
					insert_t.flags = target_flags;
				}
			}
			//As this is the end of this adding procedure, we will fill up this blank.
			if (iter == wp.end() ) {
				Push_back_wp(insert_t);
				//wp.push_back(insert_t);//a push back
				iter = wp.end();
			}
			else {
				iter = Insert_wp(insert_t, iter);
				//iter = wp.insert(iter, insert_t);//An insert
				iter++;
			}
		}
		else {//Then the end+1 address is covered by some wp. Then we'll need to test whether the flags are the same.
			//First check if there is any blank between the two nodes.
			if (iter->addr != insert_t.addr + insert_t.size) {//If there is some blank between the two nodes.
				if (insert_t.flags == target_flags)//if the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = iter->addr - insert_t.addr;
				else {//if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
					iter = Insert_wp(insert_t, iter);
					//iter = wp.insert(iter, insert_t);//An insert
					iter++;
					insert_t.addr = insert_t.addr + insert_t.size;
					insert_t.size = iter->addr - insert_t.addr;
					insert_t.flags = target_flags;
				}
			}
			//If the flag of insert_t and the up-comming node are the same, then we definetly merge the two nodes
			if (insert_t.flags == iter->flags) {
				insert_t.size = insert_t.size + iter->size;
				iter = Insert_wp(insert_t, iter);
				//iter = wp.insert(iter, insert_t);//An insert
				iter++;
				Erase_wp(iter);
				//wp.erase(iter);//Erase!
			}
			//If they aren't the same. We then need to merge part of them(maybe). And split the rest.
			else if (iter->addr <= target_addr + target_size - 1) {//If the ending address is not covered, no need to split. (Merging is done by above)
				if (flag_inclusion (target_flags, iter->flags) ) {//if the target_flag is included by iter, then we just left it as before
					iter = Insert_wp(insert_t, iter);
					//iter = wp.insert(iter, insert_t);//An insert
				}
				else {
					if (insert_t.flags == (iter->flags | target_flags) ) {//Check the merge condition.
						insert_t.size = target_addr + target_size - insert_t.addr;
						iter = Insert_wp(insert_t, iter);
						//iter = wp.insert(iter, insert_t);//An insert
						iter++;
					}
					else {
						
						//If we can't merge, we need to split the iter node
						iter = Insert_wp(insert_t, iter);
						//iter = wp.insert(iter, insert_t);//1st insert
						iter++;
						insert_t.addr = insert_t.addr + insert_t.size;
						insert_t.size = target_addr + target_size - insert_t.addr;
						insert_t.flags = iter->flags | target_flags;
						iter = Insert_wp(insert_t, iter);
						//iter = wp.insert(iter, insert_t);//2nd insert
						iter++;
					}
					//splitting(modify the node which covers the ending address of the range)
					modify_t.addr = target_addr + target_size;
					modify_t.size = iter->addr + iter->size - (target_addr + target_size);
					modify_t.flags = iter->flags;
					Modify_wp(modify_t, iter);
					//iter->size = iter->addr + iter->size - (target_addr + target_size);//It changes size
					//iter->addr = target_addr + target_size;
				}
			}
			else {
				iter = Insert_wp(insert_t, iter);
				//iter = wp.insert(iter, insert_t);
				iter++;
			}
		}
		Range_cleanup();
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::rm_watchpoint (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		if (target_size == 0) {//This is special for size = 0;
			wp.clear();
			return;
		}
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator previous_iter;
		//starting part
		iter = search_address(target_addr, wp);
		
		if (iter != wp.end() && iter->size == 0 && (iter->flags & target_flags) ) {// this is sepcial for size = 0
			if (target_addr == 0) {
				iter->addr = target_size;
				iter->size = 0 - target_size;
				if (iter->flags != target_flags) {
					watchpoint_t<ADDRESS, FLAGS> insert_t = {0, target_size, iter->flags & ~target_flags};
					wp.insert(iter, insert_t);
				}
			}
			else if (target_addr + target_size == 0) {
				iter->size = target_addr;
				if (iter->flags != target_flags) {
					watchpoint_t<ADDRESS, FLAGS> insert_t = {target_addr, target_size, iter->flags & ~target_flags};
					wp.push_back(insert_t);
				}
			}
			else {
				iter->addr = target_addr + target_size;
				iter->size = 0 - iter->addr;
				watchpoint_t<ADDRESS, FLAGS> insert_t = {0, target_addr, iter->flags};				
				if (iter->flags != target_flags) {
					watchpoint_t<ADDRESS, FLAGS> insert_T = {target_addr, target_size, iter->flags & ~target_flags};
					iter = wp.insert(iter, insert_T);
				}
				wp.insert(iter, insert_t);
			}
			return;
		}		
		
		if (iter == wp.end() )
			return;
		if (addr_covered(target_addr - 1, (*iter) ) ) {
			if (target_addr + target_size - 1 < iter->addr + iter->size - 1) {
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
		else if (target_addr + target_size - 1 < iter->addr + iter->size - 1) {
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
		while (iter != wp.end() && iter->addr + iter->size - 1 < target_addr + target_size - 1) {
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
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);
		
		rm_watchpoint (target_addr, target_size, (WA_READ | WA_WRITE) );
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
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
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);
		
		rm_watchpoint (target_addr, target_size, WA_READ);
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
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
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);
		
		rm_watchpoint (target_addr, target_size, WA_WRITE);
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change);
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
		page_break (begin_hit_before, begin_hit_after);
		page_break (end_hit_before, end_hit_after);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	bool WatchPoint<ADDRESS, FLAGS>::general_fault (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags, unsigned int& top_page, unsigned int& mid_page, unsigned int& bot_page) {
		//	cout << "Entered general_fault" << endl;

		if (target_size == 0) {
			top_page++;
			return false;
		}
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		iter = search_address(target_addr, wp);
		
		if (iter != wp.end() && iter->size == 0) {//This is special for size = 0
			top_page++;
			Range_load(0, -1);
			return (iter->flags & target_flags);
		}
		
		if (iter == wp.end() ) {
			if (iter != wp.begin() ) {
				iter--;
				if (iter->addr + iter->size -1 >= (target_addr & ~(4095) ) ) 
					bot_page++;
					
				else if (iter->addr + iter->size - 1 >= (target_addr & ~(4194303) ) )
					mid_page++;

				else
					top_page++;
					
				Range_load(iter->addr + iter->size, -1);
			}
			else {
				Range_load(0, -1);
				top_page++;
			}
			return false;
		}
		
		bool wp_fault = false;
		bool mid_level = false;
		bool bot_level = false;

		
		if(!addr_covered(target_addr, *iter) && iter !=wp.begin() ) {
			iter--;
			if (iter->addr + iter->size - 1 >= (target_addr & ~(4095) ) )
				bot_level = true;
			else if (iter->addr + iter->size - 1 >= (target_addr & ~(4194303) ) )
				mid_level = true;
			iter++;
		}
		
		ADDRESS beg_addr;
		ADDRESS end_addr;
		
		while (iter != wp.end() && iter->addr <= target_addr + target_size - 1) {
			if (iter == wp.begin() && iter->addr != 0) {
				end_addr = iter->addr - 1;
				Range_load (0, end_addr);
			}
			else if (iter != wp.begin() && (iter - 1)->addr + (iter - 1)->size != iter->addr) {
				beg_addr = (iter - 1)->addr + (iter - 1)->size;
				end_addr = iter->addr - 1;
				Range_load (beg_addr, end_addr);
			}
			beg_addr = iter->addr;
			end_addr = iter->addr + iter->size - 1;
			Range_load(beg_addr, end_addr);
			
			if (iter->flags & target_flags)
				wp_fault = true;
			
			if (iter->addr > target_addr || iter->addr > (target_addr & ~(4095) ) )
				beg_addr = iter->addr;
			else if (iter->addr > (target_addr & ~(4194303) ) )
				beg_addr = target_addr & ~(4095);
			else
				beg_addr = target_addr & ~(4194303);
				
			if ( (iter->addr + iter->size - 1 <= target_addr + target_size - 1)
			    || iter->addr + iter->size - 1 <= ( (target_addr + target_size + 4095) & ~(4095) )
			    || ( (target_addr + target_size + 4095) & ~(4095) ) == 0)
				end_addr = iter->addr + iter->size;
			
			else if ( iter->addr + iter->size - 1 <= ( (target_addr + target_size + 4194303) & ~(4194303) )
			    || ( (target_addr + target_size + 4194303) & ~(4194303) ) == 0)
				end_addr = (target_addr + target_size + 4095) & ~(4095);

			else
				end_addr = (target_addr + target_size + 4194303) & ~(4194303);
			
			if ( (beg_addr & 4095) || (end_addr & 4095) ) 
				bot_level = true;

			else if ( (beg_addr & 4194303) || (end_addr & 4194303) )
				mid_level = true;

			iter++;
		}
		
		if ( (iter != wp.end() && iter->addr < ( (target_addr + target_size + 4095) & ~(4095) ) )
		    || ( ( (target_addr + target_size + 4095) & ~(4095) ) == 0) ) {
			//	cout << "bot_modify 2" << endl;
			bot_level = true;
		}
		else if ( (iter != wp.end() && iter->addr < ( (target_addr + target_size + 4194303) & ~(4194303) ) )
		    || ( ( (target_addr + target_size + 4194303) & ~(4194303) ) == 0) ) {
			//	cout << "mid_modify 2" << endl;
			mid_level = true;
		}
		if (bot_level)
			bot_page++;
		else if (mid_level)
			mid_page++;
		else
			top_page++;
		
		//	cout << "Exited general_fault" << endl;
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
	trie_data_t WatchPoint<ADDRESS, FLAGS>::get_trie_data() {
		return trie;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::reset_trie() {
		trie = trie_data_t();
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator WatchPoint<ADDRESS, FLAGS>::Insert_wp (const watchpoint_t<ADDRESS, FLAGS>& insert_wp, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter) {
		//	cout << "Entered Insert_wp" << endl;
		ADDRESS		start_addr;
		ADDRESS		end_addr;
		range_t<ADDRESS>	insert_range;
		typename deque< range_t<ADDRESS> >::iterator	range_cache_iter;
		
		end_addr = iter->addr -1;
		
		if (iter == wp.begin() )
			start_addr = 0;
		else
			start_addr = (iter - 1)->addr + (iter - 1)->size;
		
		range_cache_iter = Range_load(start_addr, end_addr);//as insert must be insert into some kinds of blank, so there must be an unwatched range.
		range_cache_iter->start_addr = insert_wp.addr;//Change the size of the unwatched range, and make it as watched range.
		range_cache_iter->end_addr = insert_wp.addr + insert_wp.size - 1;
		if (start_addr != insert_wp.addr) {//split front
			insert_range.start_addr = start_addr;
			insert_range.end_addr = insert_wp.addr - 1;
			range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
			range.cur_range_num++;
		}
		if (end_addr != insert_wp.addr + insert_wp.size - 1) {//split back
			insert_range.start_addr = insert_wp.addr + insert_wp.size;
			insert_range.end_addr = end_addr;
			range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
			range.cur_range_num++;
		}
		iter = wp.insert(iter, insert_wp);
		//	cout << "Exited Insert_wp" << endl;
		return iter;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::Modify_wp (const watchpoint_t<ADDRESS, FLAGS>& modify_t, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter) {
		//	cout << "Entered Modify_wp" << endl;
		ADDRESS		start_addr;
		ADDRESS		end_addr;
		range_t<ADDRESS>	insert_range;
		typename deque< range_t<ADDRESS> >::iterator	range_cache_iter;
		
		if (modify_t.addr != iter->addr && iter->addr != 0) {
			end_addr = iter->addr - 1;
			if (iter == wp.begin() )
				start_addr = 0;
			else
				start_addr = (iter - 1)->addr + (iter - 1)->size;
			range_cache_iter = Range_load(start_addr, end_addr);
			if (modify_t.addr == start_addr) {
				range_cache.erase(range_cache_iter);
				range.cur_range_num--;
			}
			else
				range_cache_iter->end_addr = modify_t.addr - 1;
		}
		else if (modify_t.addr != iter->addr && iter->addr == 0) {
			insert_range.start_addr = 0;
			insert_range.end_addr = modify_t.addr - 1;
			range_cache_iter = range_cache.begin();
			range_cache.insert(range_cache_iter, insert_range);
			range.cur_range_num++;
		}
		
		start_addr = iter->addr;
		end_addr = iter->addr + iter->size - 1;
		range_cache_iter = Range_load(start_addr, end_addr);
		range_cache_iter->start_addr = modify_t.addr;
		range_cache_iter->end_addr = modify_t.addr + modify_t.size - 1;
		
		if (iter->addr + iter->size != modify_t.addr + modify_t.size && modify_t.addr + modify_t.size != 0) {
			start_addr = iter->addr + iter->size;
			if (iter == wp.end() )
				end_addr = -1;
			else
				end_addr = (iter + 1)->addr - 1;
			range_cache_iter = Range_load(start_addr, end_addr);
			if (modify_t.addr + modify_t.size - 1 == end_addr) {
				range_cache.erase(range_cache_iter);
				range.cur_range_num--;
			}
			else
				range_cache_iter->start_addr = modify_t.addr + modify_t.size - 1;
		}
		else if (iter->addr + iter->size != modify_t.addr + modify_t.size && modify_t.addr + modify_t.size == 0) {
			insert_range.start_addr = modify_t.addr + modify_t.size;
			insert_range.end_addr = -1;
			range_cache_iter = range_cache.begin();
			range_cache.insert(range_cache_iter, insert_range);
			range.cur_range_num++;
		}
		
		iter->addr = modify_t.addr;
		iter->size = modify_t.size;
		iter->flags = modify_t.flags;
		//	cout << "Exited Insert_wp" << endl;
		return;
	}
	
	template <class ADDRESS, class FLAGS>	
	void WatchPoint<ADDRESS, FLAGS>::Push_back_wp (const watchpoint_t<ADDRESS, FLAGS>& insert_wp) {
		//	cout << "Entered Push_back_wp" << endl;
		ADDRESS		start_addr;
		range_t<ADDRESS>	insert_range;
		typename deque< range_t<ADDRESS> >::iterator	range_cache_iter;
		
		if (wp.size() == 0) {// There is no watchpoint that covers the entire memory
			if (insert_wp.size == 0) {
				wp.push_back(insert_wp);
				if (insert_wp.addr != 0)
					cout << "That's a bug!!" << endl;
				return;
			}
			Range_load(0, -1);//This is used just to increment hit.
			range_cache_iter = range_cache.begin();//So the only range currently in range_cache is [0, -1]
			range_cache_iter->start_addr = insert_wp.addr;
			range_cache_iter->end_addr = insert_wp.addr + insert_wp.size - 1;
			if (insert_wp.addr != 0) {
				insert_range.start_addr = 0;
				insert_range.end_addr = insert_wp.addr - 1;
				range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
				range.cur_range_num++;
			}
			if (insert_wp.addr + insert_wp.size != 0) {
				insert_range.start_addr = insert_wp.addr + insert_wp.size - 1;
				insert_range.end_addr = -1;
				range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
				range.cur_range_num++;
			}
		}
		else {
			start_addr = (wp.end() - 1)->addr - 1;//When called push_back, there must be space at the end of "memory". So start_addr != -1
			range_cache_iter = Range_load(start_addr, -1);
			range_cache_iter->start_addr = insert_wp.addr;
			range_cache_iter->end_addr = insert_wp.addr + insert_wp.size - 1;
			if (insert_wp.addr != start_addr) {
				insert_range.start_addr = start_addr;
				insert_range.end_addr = insert_wp.addr - 1;
				range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
				range.cur_range_num++;
			}
			if (insert_wp.addr + insert_wp.size != 0) {
				insert_range.start_addr = insert_wp.addr + insert_wp.size;
				insert_range.end_addr = -1;
				range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
				range.cur_range_num++;
			}
		}
		wp.push_back(insert_wp);
		//	cout << "Exit Push_back_wp" << endl;
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	typename deque< watchpoint_t<ADDRESS, FLAGS> >::iterator WatchPoint<ADDRESS, FLAGS>::Erase_wp (typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter) {
		//	cout << "Entered Erase_wp" << endl;
		ADDRESS		start_addr;
		ADDRESS		end_addr;
		ADDRESS		ulti_start_addr;
		ADDRESS		ulti_end_addr;
		range_t<ADDRESS>	insert_range;
		typename deque< range_t<ADDRESS> >::iterator	range_cache_iter;
		
		range_cache_iter = range_cache.begin();
		//Front Blank
		end_addr = iter->addr - 1;
		
		if (iter == wp.begin())
			start_addr = 0;
		else
			start_addr = (iter - 1)->addr + (iter - 1)->size;
		ulti_start_addr = start_addr;

		if (end_addr >= start_addr) {
			range_cache_iter = Range_load(start_addr, end_addr);
			range_cache_iter = range_cache.erase(range_cache_iter);
			range.cur_range_num--;
		}
		//Middle
		start_addr = iter->addr;
		end_addr = iter->addr + iter->size - 1;
		range_cache_iter = Range_load(start_addr, end_addr);
		range_cache_iter = range_cache.erase(range_cache_iter);
		range.cur_range_num--;
		//Back Blank
		start_addr = iter->addr + iter->size;

		if (iter == wp.end())
			end_addr = -1;
		else
			end_addr = (iter + 1)->addr - 1;
		ulti_end_addr = end_addr;
		
		if (end_addr >= start_addr) {
			range_cache_iter = Range_load(start_addr, end_addr);
			range_cache_iter = range_cache.erase(range_cache_iter);
			range.cur_range_num--;
		}
		//Add the new merged unwatched range.
		insert_range.start_addr = ulti_start_addr;
		insert_range.end_addr = ulti_end_addr;
		range_cache.insert(range_cache_iter, insert_range);
		range.cur_range_num++;
		
		iter = wp.erase(iter);
		return iter;
		//	cout << "Exit Erase_wp" << endl;
	}
	
	template <class ADDRESS, class FLAGS>
	typename deque< range_t<ADDRESS> >::iterator WatchPoint<ADDRESS, FLAGS>::Range_load (ADDRESS start_addr, ADDRESS end_addr) {
		//	cout << "Range_load in" << endl;
		bool				find = false;
		typename deque< range_t<ADDRESS> >::iterator	iter;
		range_t<ADDRESS>	insert_range;
		for (iter = range_cache.begin(); iter != range_cache.end(); iter++) {
			if (iter->start_addr == start_addr) {
				find = true;
				break;
			}
		}
		if (find) {
			range.hit++;
			insert_range = *iter;
			range_cache.erase(iter);
		}
		else {
			range.miss++;
		}
		insert_range.start_addr = start_addr;
		insert_range.end_addr = end_addr;
		iter = range_cache.begin();
		iter = range_cache.insert(iter, insert_range);
		//	cout << "Range_load out" << endl;
		return iter;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::Range_cleanup() {
		if (range_cache.size() > RANGE_CACHE_SIZE) {
			typename deque< range_t<ADDRESS> >::iterator iter;
			range.kick += range_cache.size() - RANGE_CACHE_SIZE;
			for (int i = range_cache.size() - RANGE_CACHE_SIZE; i > 0; i--) {
				iter = range_cache.end() - 1;
				range_cache.erase(iter);
			}
		}
		range.changes++;
		range.avg_range_num += (range.cur_range_num - (double)range.avg_range_num) / range.changes;
		
		if (range.cur_range_num > range.max_range_num)
			range.max_range_num = range.cur_range_num;
		return;
	}
	///////////////////Below is for MEM_WatchPoint
	
	template <class ADDRESS, class FLAGS>
	MEM_WatchPoint<ADDRESS, FLAGS>::MEM_WatchPoint() {
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	MEM_WatchPoint<ADDRESS, FLAGS>::MEM_WatchPoint(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		watchpoint_t<ADDRESS, FLAGS> temp = {target_addr, target_size, target_flags};
		wp.push_back(temp);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	MEM_WatchPoint<ADDRESS, FLAGS>::MEM_WatchPoint(const MEM_WatchPoint& parameter) {
		wp.resize((int)parameter.wp.size());
		int i;
		for (i = 0; i < parameter.wp.size(); i++) {
			wp[i] = parameter.wp[i];
		}
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void MEM_WatchPoint<ADDRESS, FLAGS>::clear() {
		wp.clear();
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void MEM_WatchPoint<ADDRESS, FLAGS>::watch_print() {
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
	void MEM_WatchPoint<ADDRESS, FLAGS>::add_watch_wp (ADDRESS target_addr, ADDRESS target_size) {
		add_watchpoint (target_addr, target_size, WA_READ | WA_WRITE);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void MEM_WatchPoint<ADDRESS, FLAGS>::add_read_wp (ADDRESS target_addr, ADDRESS target_size) {
		add_watchpoint (target_addr, target_size, WA_READ);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void MEM_WatchPoint<ADDRESS, FLAGS>::add_write_wp (ADDRESS target_addr, ADDRESS target_size) {
		add_watchpoint (target_addr, target_size, WA_WRITE);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void MEM_WatchPoint<ADDRESS, FLAGS>::add_watchpoint(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
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
						if (iter->addr + iter->size > target_addr + target_size && !flag_inclusion (target_flags, iter->flags) ) {
							start_iter->size += target_size;
							iter->addr = target_addr + target_size;
							iter->size -= target_size;
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
				else if (iter->addr + iter->size > target_addr + target_size && !flag_inclusion (target_flags, iter->flags) ) {//Needs split
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
	void MEM_WatchPoint<ADDRESS, FLAGS>::rm_watchpoint (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
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
	void MEM_WatchPoint<ADDRESS, FLAGS>::rm_watch (ADDRESS target_addr, ADDRESS target_size) {
		rm_watchpoint (target_addr, target_size, (WA_READ | WA_WRITE) );
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void MEM_WatchPoint<ADDRESS, FLAGS>::rm_read (ADDRESS target_addr, ADDRESS target_size) {
		rm_watchpoint (target_addr, target_size, WA_READ);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void MEM_WatchPoint<ADDRESS, FLAGS>::rm_write (ADDRESS target_addr, ADDRESS target_size) {
		rm_watchpoint (target_addr, target_size, WA_WRITE);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	bool MEM_WatchPoint<ADDRESS, FLAGS>::general_fault (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		if (target_size == 0)
			return false;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		iter = search_address(target_addr, wp);
		if (iter == wp.end() )
			return false;
		while (iter->addr < target_addr + target_size) {
			if (iter->flags & target_flags)
				return true;
			iter++;
		}
		return false;
	}
	
	template <class ADDRESS, class FLAGS>
	bool MEM_WatchPoint<ADDRESS, FLAGS>::watch_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, (WA_READ | WA_WRITE) ) );
	}
	
	template <class ADDRESS, class FLAGS>
	bool MEM_WatchPoint<ADDRESS, FLAGS>::read_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, WA_READ) );
	}
	
	template <class ADDRESS, class FLAGS>
	bool MEM_WatchPoint<ADDRESS, FLAGS>::write_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, WA_WRITE) );
	}
	
	template <class ADDRESS, class FLAGS>
	void	MEM_WatchPoint<ADDRESS, FLAGS>::DumpStart() {
		dump_iter = wp.begin();
		return;
	}

	template <class ADDRESS, class FLAGS>
	watchpoint_t<ADDRESS, FLAGS>	MEM_WatchPoint<ADDRESS, FLAGS>::Dump() {
		watchpoint_t<ADDRESS, FLAGS> temp;
		temp = *dump_iter;
		dump_iter++;
		return temp;
	}

	template <class ADDRESS, class FLAGS>
	bool	MEM_WatchPoint<ADDRESS, FLAGS>::DumpEnd() {
		return (dump_iter == wp.end() );
	}
}

