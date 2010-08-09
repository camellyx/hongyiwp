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

// Without this macro, range cache won't be turned on.
#define       RANGE_CACHE
// Without this macro, page table checking won't be turned on.
#define       PAGE_TABLE

#define RANGE_CACHE_SIZE	64
#define WLB_SIZE 128

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

#ifdef RANGE_CACHE
	template<class ADDRESS>
	struct range_t {
        ADDRESS	start_addr;
        ADDRESS	end_addr;
        bool dirty;
	};
#endif

	struct trie_data_t {
		unsigned long long top_hit;
		unsigned long long mid_hit;
		unsigned long long bot_hit;
		
		unsigned long long top_change;
		unsigned long long mid_change;
		unsigned long long bot_change;
		
		unsigned long long top_break;
		unsigned long long mid_break;

        unsigned long long wlb_hit_top;
        unsigned long long wlb_hit_mid;
        unsigned long long wlb_hit_bot;

        unsigned long long wlb_miss_top;
        unsigned long long wlb_miss_mid;
        unsigned long long wlb_miss_bot;
		
		const trie_data_t operator+(const trie_data_t &other) const;
		trie_data_t();
	};

#ifdef RANGE_CACHE
	struct range_data_t {
		unsigned long long max_range_num;
        unsigned long long total_cur_range_num;
		unsigned long long cur_range_num;
		unsigned long long changes;
		unsigned long long hit;
		unsigned long long miss;
		unsigned long long kick;
        unsigned long long dirty_kick;
        unsigned int avg_range_num;
		
		const range_data_t operator+(const range_data_t &other) const;
		range_data_t();
	};
#endif

#ifdef PAGE_TABLE
	struct pagetable_data_t {
		unsigned long long access;
		unsigned long long wp_hit;
		const pagetable_data_t operator+(const pagetable_data_t &other) const;
		pagetable_data_t();
	};
#endif
	
	trie_data_t::trie_data_t() {
		top_hit = 0;
		mid_hit = 0;
		bot_hit = 0;
		
		top_change = 0;
		mid_change = 0;
		bot_change = 0;
		
		top_break = 0;
		mid_break = 0;

        wlb_hit_top = 0;
        wlb_hit_mid = 0;
        wlb_hit_bot = 0;

        wlb_miss_top = 0;
        wlb_miss_mid = 0;
        wlb_miss_bot = 0;
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
        result.wlb_hit_top += other.wlb_hit_top;
        result.wlb_hit_mid += other.wlb_hit_mid;
        result.wlb_hit_bot += other.wlb_hit_bot;
        result.wlb_miss_top += other.wlb_miss_top;
        result.wlb_miss_mid += other.wlb_miss_mid;
        result.wlb_miss_bot += other.wlb_miss_bot;
		return result;
	}
	
#ifdef RANGE_CACHE
	const range_data_t range_data_t::operator+(const range_data_t &other) const {
		range_data_t result = *this;
        result.max_range_num += other.max_range_num;
        result.total_cur_range_num += other.total_cur_range_num;
        result.cur_range_num += other.cur_range_num;
		result.changes += other.changes;
		result.hit += other.hit;
		result.miss += other.miss;
		result.kick += other.kick;
        result.dirty_kick += other.dirty_kick;
		return result;
	}
	
	range_data_t::range_data_t() {
		max_range_num = 1;
        total_cur_range_num = 1;
		cur_range_num = 1;
		changes = 1;
		hit = 0;
		miss = 0;
		kick = 0;
        dirty_kick = 0;
	}
#endif

#ifdef PAGE_TABLE
	pagetable_data_t::pagetable_data_t() {
		access = 0;
		wp_hit = 0;
	}
	
	const pagetable_data_t pagetable_data_t::operator+(const pagetable_data_t &other) const {
		pagetable_data_t result = *this;
		result.access += other.access;
		result.wp_hit += other.wp_hit;
		return result;
	}
#endif

	enum PAGE_HIT {TOP, MID, BOT};
	
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
		//reset trie;
#ifdef RANGE_CACHE
		range_data_t get_range_data();
#endif

#ifdef PAGE_TABLE
		pagetable_data_t get_pagetable_data();
#endif
		//return: The range data;
		
		
		void	watch_print();
		
		//all below are private
		void rm_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
        void invalidate_wlb (ADDRESS target_addr, ADDRESS target_size);
		void add_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		bool general_fault	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags,  unsigned long long& top_page, unsigned long long& mid_page, unsigned long long& bot_page, bool lookaside, bool hit_miss_care);
		PAGE_HIT page_level	(ADDRESS target_addr, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter);
		void page_break		(PAGE_HIT& before, PAGE_HIT& after);
		void page_break_same_top (PAGE_HIT& before, PAGE_HIT& after_1, PAGE_HIT& after_2);
		void page_break_same_mid (PAGE_HIT& before, PAGE_HIT& after_1, PAGE_HIT& after_2);
		
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator	Insert_wp	(const watchpoint_t<ADDRESS, FLAGS>& insert_t, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter);//This would call wp.insert() internally and forward the return value out. But meanwhile it would emulate range cache access
		void														Modify_wp	(const watchpoint_t<ADDRESS, FLAGS>& modify_t, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter);//This would change the value of iter's node. And emualte range cache_access
		void														Push_back_wp	(const watchpoint_t<ADDRESS, FLAGS>& insert_t);//This would call wp.push_back() internally and emulate range_cache access
		typename deque< watchpoint_t<ADDRESS, FLAGS> >::iterator	Erase_wp	(typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter);//This would call wp.erase() internally and forward the return value out. Meanwhile emulate range_cache access
#ifdef RANGE_CACHE
        typename deque< range_t<ADDRESS> >::iterator				Range_load	(ADDRESS start_addr,	ADDRESS end_addr, bool hit_miss_care);//
#endif
		//Func:		Search within the range_cache, and automatically increment hits and misses.
		//return:	the iterator that points to the range_t that is [start_addr, end_addr]. If the searched range is not within cache it would be insert at the front(The most recently used).
#ifdef RANGE_CACHE
		void														Range_cleanup();
		//Func:		This function would be called in Insert_wp, Modify_wp, Push_back_wp and Erase_wp. It would check if after these calling, is range_cache has overflow? If there are, then increment the kick number.
#endif
		deque< watchpoint_t<ADDRESS, FLAGS> >	wp;
#ifdef RANGE_CACHE
		deque< range_t<ADDRESS> >	range_cache;
		range_data_t	range;
#endif

#ifdef PAGE_TABLE
		pagetable_data_t		pagetable;
#endif
		trie_data_t		trie;
        deque< watchpoint_t<ADDRESS, FLAGS> > wlb;
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
		//		return true if dump_iter reaches the end of wp.
		
		void	watch_print();
		
		void rm_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		void add_watchpoint	(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		bool general_fault (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags);
		deque< watchpoint_t<ADDRESS, FLAGS> > wp;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator dump_iter;
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
	
    // Search for target_addr in the watchpoint system &wp.  If target_addr is
    // located in any watchpoint, return an iterator to that watchpoint.
    // Else, return an iterator to the NEXT watchpoint.
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
#ifdef RANGE_CACHE
		//Intiallize the range cache.
		range_t<ADDRESS>	insert_range = {0, -1};
		range_cache.push_back(insert_range);
#endif
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	WatchPoint<ADDRESS, FLAGS>::WatchPoint(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		watchpoint_t<ADDRESS, FLAGS> temp = {target_addr, target_size, target_flags};
		wp.push_back(temp);
#ifdef RANGE_CACHE
		//Intiallize the range cache.
		range_t<ADDRESS>	insert_range = {target_addr, target_addr + target_size - 1};
		range_cache.push_back(insert_range);
		if (target_addr != 0) {
			insert_range.start_addr = 0;
			insert_range.end_addr = target_addr - 1;
            insert_range.dirty = false;
			range_cache.push_back(insert_range);
			range.cur_range_num++;
		}
		if (target_addr + target_size != 0) {
			insert_range.start_addr = target_addr + target_size;
			insert_range.end_addr = -1;
            insert_range.dirty = false;
			range_cache.push_back(insert_range);
			range.cur_range_num++;
		}
#endif
		return;
	}

	template <class ADDRESS, class FLAGS>
	WatchPoint<ADDRESS, FLAGS>::WatchPoint(const WatchPoint& parameter) {
        wp.clear();
		wp.resize((unsigned long long)parameter.wp.size() );
		unsigned long long i;
		for (i = 0; i < parameter.wp.size(); i++) {
			wp[i] = parameter.wp[i];
		}
		trie = parameter.trie();
#ifdef RANGE_CACHE
        range_cache.clear();
		range_cache.resize ((unsigned long long)parameter.range_cache.size() );
		for (i = 0; i < parameter.range_cache.size(); i++) {
			range_cache[i] = parameter.range_cache[i];
		}
		range = parameter.range;
#endif
		return;
	}

	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::watch_print() {
        unsigned int i;
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
	
	//If start_addr and end_addr are in the same mid, then any break could happen once.
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::page_break_same_mid (PAGE_HIT& before, PAGE_HIT& after_1, PAGE_HIT& after_2) {
		if (before == TOP && (after_1 != TOP || after_2 != TOP) )
			trie.top_break++;
		else if (before == MID && (after_1 == BOT || after_2 == BOT) )
			trie.mid_break++;
		return;
	}
	
	//If start_addr and end_addr share the same top but not the same mid, then top break can happen once but mid break may happen twice!
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::page_break_same_top (PAGE_HIT& before, PAGE_HIT& after_1, PAGE_HIT& after_2) {
		if (before == TOP && (after_1 != TOP || after_2 != TOP) )
			trie.top_break++;
		else if (before == MID) {
			if (after_1 == BOT)
				trie.mid_break++;
			if (after_2 == BOT)
				trie.mid_break++;
		}
		return;
	}
		
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::add_watch_wp (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		bool same_range_flag = false;//This flag would check if both beg_addr and end_addr fall into the same RANGE(both watched or unwatched and beg_iter = end_iter)
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		if (beg_iter == end_iter && (beg_iter == wp.end() || addr_covered(target_addr, *beg_iter) == addr_covered(target_addr + target_size - 1, *end_iter) ) )
			same_range_flag = true;
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);
		
		add_watchpoint (target_addr, target_size, WA_READ | WA_WRITE);
#ifdef	RANGE_CACHE
		range_data_t temp = range;
#endif
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change, false, false);
#ifdef	RANGE_CACHE
		range = temp;//To prevent double counting for range cache hits.
#endif
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
		if (same_range_flag && (target_addr & ~(4095) ) == ( (target_addr + target_size - 1) & ~(4095) ) )//Check if they are in the same mid page first.
			page_break_same_mid (begin_hit_before, begin_hit_after, end_hit_after);
		else if (same_range_flag && (target_addr & ~(4194303) ) == ( (target_addr + target_size - 1) & ~(4194303) ) )
			page_break_same_top (begin_hit_before, begin_hit_after, end_hit_after);//To prevent double counting for top break if start
		else {
			page_break (begin_hit_before, begin_hit_after);
			page_break (end_hit_before, end_hit_after);
		}
		
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::add_read_wp (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		bool same_range_flag = false;
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		if (beg_iter == end_iter && (beg_iter == wp.end() || addr_covered(target_addr, *beg_iter) == addr_covered(target_addr + target_size - 1, *end_iter) ) )
			same_range_flag = true;
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);
		
		add_watchpoint (target_addr, target_size, WA_READ);
#ifdef	RANGE_CACHE
		range_data_t temp = range;
#endif
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change, false, false);
#ifdef	RANGE_CACHE
		range = temp;
#endif
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
		if (same_range_flag && (target_addr & ~(4095) ) == ( (target_addr + target_size - 1) & ~(4095) ) )//Check if they are in the same mid page first.
			page_break_same_mid (begin_hit_before, begin_hit_after, end_hit_after);
		else if (same_range_flag && (target_addr & ~(4194303) ) == ( (target_addr + target_size - 1) & ~(4194303) ) )
			page_break_same_top (begin_hit_before, begin_hit_after, end_hit_after);//To prevent double counting for top break if start
		else {
			page_break (begin_hit_before, begin_hit_after);
			page_break (end_hit_before, end_hit_after);
		}
		
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::add_write_wp (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		bool same_range_flag = false;
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		if (beg_iter == end_iter && (beg_iter == wp.end() || addr_covered(target_addr, *beg_iter) == addr_covered(target_addr + target_size - 1, *end_iter) ) )
			same_range_flag = true;
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);
		
		add_watchpoint (target_addr, target_size, WA_WRITE);
#ifdef	RANGE_CACHE
		range_data_t temp = range;
#endif
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change, false, false);
#ifdef	RANGE_CACHE
		range = temp;
#endif
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
		if (same_range_flag && (target_addr & ~(4095) ) == ( (target_addr + target_size - 1) & ~(4095) ) )//Check if they are in the same mid page first.
			page_break_same_mid (begin_hit_before, begin_hit_after, end_hit_after);
		else if (same_range_flag && (target_addr & ~(4194303) ) == ( (target_addr + target_size - 1) & ~(4194303) ) )
			page_break_same_top (begin_hit_before, begin_hit_after, end_hit_after);//To prevent double counting for top break if start
		else {
			page_break (begin_hit_before, begin_hit_after);
			page_break (end_hit_before, end_hit_after);
		}
		
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::add_watchpoint(ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		watchpoint_t<ADDRESS, FLAGS> insert_t = {0, 0, 0};
		watchpoint_t<ADDRESS, FLAGS> modify_t = {0, 0, 0};

		if (target_size == 0) {//This is special for size = 0. As it clears all the current watchpoints and add the whole system as watched.
            watchpoint_t<ADDRESS, FLAGS> temp_val;
			wp.clear();
			insert_t.flags = target_flags;
#ifdef RANGE_CACHE
			range_cache.clear();
			range_t<ADDRESS> insert_range = {0, -1};
			range_cache.push_back(insert_range);
            range.cur_range_num = 1;
#endif
			Push_back_wp(insert_t);

            temp_val.addr = 0;
            temp_val.size = -1;
            temp_val.flags = WA_WRITE|WA_READ;
            wlb.clear();
            wlb.push_front(temp_val);
			return;
		}
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator start_iter;//This one is used only for merging the front wp nodes.

        invalidate_wlb(target_addr, target_size);

		iter = search_address(target_addr, wp);
		
		if (iter != wp.end() && iter->size == 0) {// This is special for size = 0. It would then check for flag inclusion and split it if necessary.
            // If the flags between the all-memory Iterator and the newly-added
            // watchpoint are different, we need to either update the old WP,
            // or split the all-memory WP into multiple pieces.
            // NOTE: We do not need to do Range Cache cleanups here because this
            //       yields, at max, 3 ranges. We assume more than that.
			if (!flag_inclusion (target_flags, iter->flags) ) {
				if (target_addr == 0) {
                    // If the new region is at the beginning of memory, split WP in two.
					insert_t.addr = target_addr + target_size;
					insert_t.size = 0 - insert_t.addr;
					insert_t.flags = iter->flags;
					modify_t.addr = iter->addr;
					modify_t.size = target_size;
					modify_t.flags = target_flags | iter->flags;
                   	Modify_wp(modify_t,iter);
					Push_back_wp(insert_t);
				}
				else if (target_addr + target_size == 0) {
                    // If the new region is at the end of memory, split WP in two.
					insert_t.addr = target_addr;
					insert_t.size = target_size;
					insert_t.flags = iter->flags | target_flags;
					modify_t.addr =iter->addr;
					modify_t.size = target_addr;
					modify_t.flags = iter->flags;
					Modify_wp(modify_t, iter);
					Push_back_wp(insert_t);
				}
				else {
                    // If the new region is in the middle, split WP into three.
					insert_t.addr = 0;
					insert_t.size = target_addr;
					insert_t.flags = iter->flags;
					modify_t.addr = target_addr;
					modify_t.size = target_size;
					modify_t.flags = target_flags | iter->flags;
					Modify_wp(modify_t, iter);
					iter = Insert_wp(insert_t, iter);
					insert_t.addr = target_addr + target_size;
					insert_t.size = 0 - insert_t.addr;
					insert_t.flags = iter->flags;
					Push_back_wp(insert_t);
				}
			}
			return;
		}
		
		if (iter == wp.end() ) {
            // Not found, and the last WP doesn't even reach our address.
			if (iter != wp.begin() ) {
                // Check if there is some WP before iter; if there is, we potentially need merge them into one.
				start_iter = iter - 1;
				if (start_iter->addr + start_iter->size == target_addr && start_iter->flags == target_flags) {
                    //Merge condition.
					modify_t.addr = start_iter->addr;
					modify_t.size = start_iter->size + target_size;
					modify_t.flags = start_iter->flags;
					Modify_wp(modify_t, start_iter);
#ifdef RANGE_CACHE
					Range_cleanup();
#endif
					return;
				}
			}
            // Let's insert the watchpoint at the end.
			insert_t.addr = target_addr;
			insert_t.size = target_size;
			insert_t.flags = target_flags;
			Push_back_wp(insert_t);
#ifdef RANGE_CACHE
			Range_cleanup();
#endif
			return;
		}

		if (addr_covered (target_addr, (*iter) ) ) {
            // Our current range is covered by a watchpoint
			if (iter->addr == target_addr) {
                //We will only need to consider merging if this is true.
				if (iter != wp.begin() ) {
					start_iter = iter - 1;
					if(start_iter->addr + start_iter->size == target_addr && start_iter->flags == (iter->flags | target_flags) ) {
                        //We will have to merge the two node.
						if (iter->addr + iter->size - 1 > target_addr + target_size - 1 && !flag_inclusion (target_flags, iter->flags) ) {
                            // If the thing we actually hit encompasses this watchpoint, we must split it.
                            // Make previous longer
							modify_t.addr = start_iter->addr;
							modify_t.size = start_iter->size + target_size;
							modify_t.flags = start_iter->flags;
							Modify_wp(modify_t, start_iter);
							// Make actual hit smaller
                            modify_t.addr = target_addr + target_size;
							modify_t.size = iter->size - target_size;
							modify_t.flags = iter->flags;
							Modify_wp(modify_t, iter);
#ifdef RANGE_CACHE
							Range_cleanup();
#endif
							return;
						}
                        // If the current insertion is bigger than what it hit
                        // Just delete the old two guys and replace it with a large range.
                        // If you have [start_iter][iter][third] and your added range
                        // overflows into third (or more), the iterating code below covers it.
						insert_t.addr = start_iter->addr;
						insert_t.flags = start_iter->flags;
						insert_t.size = start_iter->size + iter->size;
                        // Erase old guys.
						iter = Erase_wp(start_iter);
						iter = Erase_wp(iter);
					}
					else {
                        // They don't touch, so there's no merging to take place.
						insert_t.addr = iter->addr;
						insert_t.flags = iter->flags | target_flags;
						if (iter->addr + iter->size - 1 > target_addr + target_size - 1 && !flag_inclusion (target_flags, iter->flags) ) {
                            // We will need to split the range into two if iter encompasses the insertion & flags differ.
							modify_t.addr = target_addr + target_size;
							modify_t.size = iter->size - target_size;
							modify_t.flags = iter->flags;
							Modify_wp(modify_t, iter);
                            insert_t.size = target_size;
							Insert_wp(insert_t, iter);
#ifdef RANGE_CACHE
							Range_cleanup();
#endif
							return;
						}
                        // Remove old guy because we'll add the new guy below.
						insert_t.size = iter->size;
						iter = Erase_wp(iter);
					}
				}
				else if (iter->addr + iter->size - 1 > target_addr + target_size - 1 && !flag_inclusion (target_flags, iter->flags) ) {
                    // Nobody before us.  We are completedly encompassed, so split into 2.
					modify_t.addr = target_addr + target_size;
					modify_t.size = iter->size - target_size;
					modify_t.flags = iter->flags;
					Modify_wp(modify_t, iter);
					insert_t.addr = target_addr;
					insert_t.size = target_size;
					insert_t.flags = target_flags | iter->flags;
					Insert_wp(insert_t, iter);
#ifdef RANGE_CACHE
					Range_cleanup();
#endif
					return;
				}
				else {
                    // Just 'extend' the range of the old guy below.  Get rid of him here.
					insert_t.addr = iter->addr;
					insert_t.size = iter->size;
					insert_t.flags = iter->flags | target_flags;
					iter = Erase_wp(iter);
				}
			}
			else {
                // We aren't aligned with the front of our hit range.
                // We will need to consider splitting.
				if (flag_inclusion (target_flags, iter->flags) ) {
                    //if the flag is included
					//then just mark the node as "to be inserted"
					insert_t.addr = iter->addr;
					insert_t.size = iter->size;
					insert_t.flags = iter->flags;
					iter = Erase_wp(iter);
					//!!As this watchpoint node is erased, iter automatically increments by 1.//Erase!
				}
				else {
                    //if the flag is not included, we then need to split the watchpoint
					if (iter->addr + iter->size - 1 > target_addr + target_size - 1) {
                        // Completely contained within the iter watchpoint, so split into 3.
                        // Insert third partition.
						insert_t.size = iter->addr + iter->size - target_addr - target_size;
                        insert_t.addr = target_addr + target_size;
						insert_t.flags = iter->flags;
                        // Modify current to become first partition.
						modify_t.addr = iter->addr;
						modify_t.size = target_addr - iter->addr;
						modify_t.flags = iter->flags;
						Modify_wp(modify_t, iter);
						// split the watchpoint by modifying iter's length. It changes size.
						iter++;
						iter = Insert_wp(insert_t, iter);
                        // Insert the second partition.
						insert_t.addr = target_addr;
						insert_t.size = target_size;
						insert_t.flags = target_flags | iter->flags;
						iter = Insert_wp(insert_t, iter);
						return;
					}
                    // This guy flies off the end of the current watchpoint.  Just shrink the beginning and add WP at the end.
					insert_t.size = iter->addr + iter->size - target_addr;
					insert_t.addr = target_addr;
					insert_t.flags = target_flags | iter->flags;
					modify_t.addr = iter->addr;
					modify_t.size = target_addr - iter->addr;
					modify_t.flags = iter->flags;
					Modify_wp(modify_t, iter);
					// split the watchpoint by modifying iter's length. It changes size.
					iter++;//increment to the next node
				}
			}
		}
		else {
            // The target_addr is not covered at all by the iterator.
			if (iter != wp.begin()) {
                // Check to see if the WP to be added should be merged with a WP before it.
				start_iter = iter - 1;
				if (start_iter->addr + start_iter->size == target_addr && start_iter->flags == target_flags) {
                    // Merge them!
					insert_t.addr = start_iter->addr;
					if (iter->addr <= target_addr + target_size - 1) // Need to potentially merge end with iter below
						insert_t.size = iter->addr - insert_t.addr;
					else
						insert_t.size = target_size + target_addr - insert_t.addr; // Add insert onto the end
					insert_t.flags = target_flags;
					iter = Erase_wp(start_iter); // Erase old guy
				}
				else {
                    // Else we will have just a regular insertion.
					insert_t.addr = target_addr;
					if (iter->addr <= target_addr + target_size - 1) // Need to potentially merge end with iter below
						insert_t.size = iter->addr - target_addr;
					else
						insert_t.size = target_size;
					insert_t.flags = target_flags;
				}
			}
			else {
                // This should become the first watchpoint in the system.
                // May need to merge the end, but not the beginning.
				insert_t.addr = target_addr;
				if (iter->addr <= target_addr + target_size - 1) // Need to otentially merge end with iter below
					insert_t.size = iter->addr - target_addr;
				else
					insert_t.size = target_size;
				insert_t.flags = target_flags;
			}
		}
		
		
		//Iterating part--If we are inserting something that covers multiple old watchpoints,
        //we must appropriately merge them all together.
		while (iter != wp.end() && iter->addr + iter->size - 1 <= target_addr + target_size - 1) {
            // We only need to perform this while our insertion guy reaches past the end of each iterator
			if (iter->addr != insert_t.addr + insert_t.size) {
                //If there is some blank between our current insert node start address and the iterator
				if (insert_t.flags == target_flags) {
                    //if the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = iter->addr - insert_t.addr;
                }
				else {
                    //if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
					iter = Insert_wp(insert_t, iter);
					iter++;
					insert_t.addr = insert_t.addr + insert_t.size;
					insert_t.size = iter->addr - insert_t.addr;
					insert_t.flags = target_flags;
				}
			}
			if (insert_t.flags == (iter->flags | target_flags) ) {
                //then we will need to merge the two node, by just enlarging the insert_t size
				insert_t.size = insert_t.size + iter->size;
            }
			else {
                //if not, then we will first insert insert_t into wp, and update insert_t.
				iter = Insert_wp(insert_t, iter);
				iter++;
				insert_t.addr = iter->addr;
				insert_t.size = iter->size;
				insert_t.flags = iter->flags | target_flags;
			}
			iter = Erase_wp(iter);
			// We must delete the WP we walked through, since we will add them back in the future.
        }
		
		//Ending part - Insert the last large watchpoint.
		if (iter == wp.end() || !addr_covered( (target_addr + target_size), (*iter) ) ) {
            //If it's the end of the deque or the end+1 address is not covered by any wp
			//Then we simply add the watchpoint onto the end.
			if (target_addr + target_size - 1 > insert_t.addr + insert_t.size - 1) {
                //If there is still some blank between insert_t and the end of the adding address.
				if (insert_t.flags == target_flags) {
                    //if the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = target_addr + target_size - insert_t.addr;
				}
				else {
                    //if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
					if (iter == wp.end() ) {
						Push_back_wp(insert_t);
						iter = wp.end();
					}
					else {
						iter = Insert_wp(insert_t, iter);
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
				iter = wp.end();
			}
			else {
				iter = Insert_wp(insert_t, iter);
				iter++;
			}
		}
		else {
            //Then the end+1 address is covered by some wp. Then we'll need to test whether the flags are the same.
			//First check if there is any blank between the two nodes.
			if (iter->addr != insert_t.addr + insert_t.size) {
                //If there is some blank between the two nodes.
				if (insert_t.flags == target_flags) {
                    //if the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = iter->addr - insert_t.addr;
                }
				else {
                    //if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
					iter = Insert_wp(insert_t, iter);
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
				iter++;
				Erase_wp(iter);
			}
			//If they aren't the same. We then need to merge part of them(maybe). And split the rest.
			else if (iter->addr <= target_addr + target_size - 1) {
                //If the ending address is not covered, no need to split. (Merging is done by above)
				if (flag_inclusion (target_flags, iter->flags) ) {
                    //if the target_flag is included by iter, then we just left it as before
					iter = Insert_wp(insert_t, iter);
				}
				else {
					if (insert_t.flags == (iter->flags | target_flags) ) {//Check the merge condition.
						insert_t.size = target_addr + target_size - insert_t.addr;
						iter = Insert_wp(insert_t, iter);
						iter++;
					}
					else {
						
						//If we can't merge, we need to split the iter node
						iter = Insert_wp(insert_t, iter);
						iter++;
						insert_t.addr = insert_t.addr + insert_t.size;
						insert_t.size = target_addr + target_size - insert_t.addr;
						insert_t.flags = iter->flags | target_flags;
						iter = Insert_wp(insert_t, iter);
						iter++;
					}
					//splitting(modify the node which covers the ending address of the range)
					modify_t.addr = target_addr + target_size;
					modify_t.size = iter->addr + iter->size - (target_addr + target_size);
					modify_t.flags = iter->flags;
					Modify_wp(modify_t, iter);
				}
			}
			else {
				iter = Insert_wp(insert_t, iter);
				iter++;
			}
		}
#ifdef RANGE_CACHE
		Range_cleanup();
#endif
		return;
	}

    template <class ADDRESS, class FLAGS>
    void WatchPoint<ADDRESS, FLAGS>::invalidate_wlb (ADDRESS target_addr, ADDRESS target_size) {
        typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
        for (iter = wlb.begin(); iter != wlb.end(); ) {
            if (((iter->addr + iter->size) > target_addr) && (iter->addr < (target_addr + target_size))) {
                // Some overlap occurred
                iter = wlb.erase(iter);
            }
            else
                iter++;
        }
    }
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::rm_watchpoint (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags) {
		watchpoint_t<ADDRESS, FLAGS> modify_t = {0, 0, 0};

		if (target_size == 0) {//This is special for size = 0;
            watchpoint_t<ADDRESS, FLAGS> temp_val;
			wp.clear();

            temp_val.addr = 0;
            temp_val.size = -1;
            temp_val.flags = WA_WRITE | WA_READ;
            wlb.clear();
            wlb.push_front(temp_val);
#ifdef RANGE_CACHE
			range_cache.clear();
			range_t<ADDRESS> insert_range = {0, -1};
			range_cache.push_back(insert_range);
			range.cur_range_num = 1;
#endif
			return;
		}
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator previous_iter;

        invalidate_wlb(target_addr, target_size);

		//starting part
		iter = search_address(target_addr, wp);
	    
		if (iter != wp.end() && iter->size == 0 && (iter->flags & target_flags) ) {
            // this is special for size = 0.  We're deleting the "all memory covered" watchpoint.
			if (target_addr == 0) {
				modify_t.addr = target_size;
				modify_t.size = 0 - target_size;
				modify_t.flags = iter->flags;
				Modify_wp(modify_t, iter);
				if (iter->flags != target_flags) {
					watchpoint_t<ADDRESS, FLAGS> insert_t = {0, target_size, iter->flags & ~target_flags};
					Insert_wp(insert_t, iter);
				}
			}
			else if (target_addr + target_size == 0) {
				modify_t.addr = iter->addr;
				modify_t.size = target_addr;
				modify_t.flags = iter->flags;
				Modify_wp(modify_t, iter);
				if (iter->flags != target_flags) {
					watchpoint_t<ADDRESS, FLAGS> insert_t = {target_addr, target_size, iter->flags & ~target_flags};
					Push_back_wp(insert_t);
				}
			}
			else {
				modify_t.addr = target_addr + target_size;
				modify_t.size = 0 - modify_t.addr;
				modify_t.flags = iter->flags;
				Modify_wp(modify_t, iter);
				watchpoint_t<ADDRESS, FLAGS> insert_t = {0, target_addr, iter->flags};				
				if (iter->flags != target_flags) {
					watchpoint_t<ADDRESS, FLAGS> insert_T = {target_addr, target_size, iter->flags & ~target_flags};
					iter = Insert_wp(insert_T, iter);
				}
				Insert_wp(insert_t, iter);
			}
#ifdef RANGE_CACHE
			Range_cleanup();
#endif
			return;
		}
		
		if (iter == wp.end() ) {
#ifdef RANGE_CACHE
			Range_cleanup();
#endif
			return;
		}
		if (addr_covered(target_addr - 1, (*iter) ) ) {
            // Check to see if the iterator needs split.  If it starts at least one address before the guy we're adding, we must split.
			if (target_addr + target_size - 1 < iter->addr + iter->size - 1) {
				if (target_flags & iter->flags) {
					watchpoint_t<ADDRESS, FLAGS> insert_t = {iter->addr, target_addr - iter->addr, iter->flags};
					modify_t.addr = target_addr + target_size;
					modify_t.size = iter->addr + iter->size - target_addr - target_size;
					modify_t.flags = iter->flags;
					Modify_wp(modify_t, iter);
					iter = Insert_wp(insert_t, iter);
					insert_t.flags = iter->flags & (~target_flags);
					if (insert_t.flags) {
						insert_t.addr = target_addr;
						insert_t.size = target_size;
						iter++;
						Insert_wp(insert_t, iter);
					}
				}
#ifdef RANGE_CACHE
				Range_cleanup();
#endif
				return;
			}
			if (target_flags & iter->flags) {
				watchpoint_t<ADDRESS, FLAGS> insert_t = {target_addr, iter->addr + iter->size - target_addr, iter->flags & (~target_flags)};
				modify_t.addr = iter->addr;
				modify_t.size = target_addr - iter->addr;
				modify_t.flags = iter->flags;
				Modify_wp(modify_t, iter);
				iter++;
				if (insert_t.flags) {
					iter = Insert_wp(insert_t, iter);
					iter++;
				}
			}
		}
		else if (target_addr + target_size - 1 < iter->addr + iter->size - 1) {
			if (iter->flags & target_flags) {
				watchpoint_t<ADDRESS, FLAGS> insert_t = {iter->addr, target_addr + target_size - iter->addr, iter->flags & (~target_flags)};
				modify_t.addr = target_addr + target_size;
				modify_t.size = iter->addr + iter->size - target_addr - target_size;
				modify_t.flags = iter->flags;
				Modify_wp(modify_t, iter);
				//iter->size = iter->addr + iter->size - target_addr - target_size;
				//iter->addr = target_addr + target_size;
				if (insert_t.flags) {
					previous_iter = iter - 1;
					if (previous_iter->addr + previous_iter->size == insert_t.addr && previous_iter->flags == insert_t.flags) {
						modify_t.addr = previous_iter->addr;
						modify_t.size = previous_iter->size + insert_t.size;
						modify_t.flags = previous_iter->flags;
						Modify_wp(modify_t, previous_iter);
					}
					else {
						iter = Insert_wp(insert_t, iter);
						iter++;
					}
				}
			}
#ifdef RANGE_CACHE
			Range_cleanup();
#endif
			return;
		}

		//iterating part		
		while (iter != wp.end() && iter->addr + iter->size - 1 < target_addr + target_size - 1) {
			if (iter->flags & target_flags) {
				modify_t.addr = iter->addr;
				modify_t.size = iter->size;
				modify_t.flags = iter->flags & (~target_flags);
				Modify_wp(modify_t, iter);
				if (iter->flags) {
					previous_iter = iter - 1;
					if (previous_iter->addr + previous_iter->size == iter->addr && previous_iter->flags == iter->flags) {
						modify_t.addr = previous_iter->addr;
						modify_t.size = previous_iter->size + iter->size;
						modify_t.flags = previous_iter->flags;
						Modify_wp(modify_t, previous_iter);
						iter = Erase_wp(iter);
					}
					else
						iter++;
				}
				else
					iter = Erase_wp(iter);
			}
			else
				iter++;
		iter++;
		}
		
		//ending part
		if (iter != wp.end() && addr_covered( (target_addr + target_size - 1), (*iter) ) && iter->flags & target_flags) {
			if (target_addr + target_size == iter->addr + iter->size) {
				modify_t.addr = iter->addr;
				modify_t.size = iter->size;
				modify_t.flags = iter->flags & (~target_flags);
				Modify_wp(modify_t, iter);
				//iter->flags = iter->flags & (~target_flags);
				if (iter->flags) {
					previous_iter = iter - 1;
					if (previous_iter->addr + previous_iter->size == iter->addr && previous_iter->flags == iter->flags) {
						modify_t.addr = previous_iter->addr;
						modify_t.size = previous_iter->size + iter->size;
						modify_t.flags = previous_iter->flags;
						Modify_wp(modify_t, previous_iter);
						//previous_iter->size += iter->size;
				 		iter = Erase_wp(iter);
				 	}
				}
				else
					Erase_wp(iter);
			}
			else {
				watchpoint_t<ADDRESS, FLAGS> insert_t = {iter->addr , target_addr + target_size - iter->addr, iter->flags & (~target_flags)};
				if (insert_t.flags) {
					previous_iter = iter - 1;
					if (previous_iter->addr + previous_iter->size == insert_t.addr && previous_iter->flags == insert_t.flags) {
						modify_t.addr = previous_iter->addr;
						modify_t.size = previous_iter->size + insert_t.size;
						modify_t.flags = previous_iter->flags;
						Modify_wp(modify_t, iter);
						//previous_iter->size += insert_t.size;
					}
					else {
						iter = Insert_wp(insert_t, iter);
						iter++;
					}
				}
				modify_t.addr = target_addr + target_size;
				modify_t.size = iter->addr + iter->size - target_addr - target_size;
				modify_t.flags = iter->flags;
				Modify_wp(modify_t, iter);
				//iter->size = iter->addr + iter->size - target_addr - target_size;
				//iter->addr = target_addr + target_size;
			}
		}
		previous_iter = iter - 1;
		if (iter != wp.end() && previous_iter->addr + previous_iter->size == iter->addr && previous_iter->flags == iter->flags) {
 			modify_t.addr = previous_iter->addr;
 			modify_t.size = previous_iter->size + iter->size;
 			modify_t.flags = previous_iter->flags;
 			Modify_wp(modify_t, previous_iter);
 			//previous_iter->size += iter->size;
 			iter = Erase_wp(iter);
 		}
#ifdef RANGE_CACHE
 		Range_cleanup();
#endif
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::rm_watch (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		bool same_range_flag = false;
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		if (beg_iter == end_iter && (beg_iter == wp.end() || addr_covered(target_addr, *beg_iter) == addr_covered(target_addr + target_size - 1, *end_iter) ) )
			same_range_flag = true;
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);

        // Removing an entire watchpoint is not an update, so we can ignore loading stuff into the lookaside buffer & range cache
		
		rm_watchpoint (target_addr, target_size, (WA_READ | WA_WRITE) );
#ifdef	RANGE_CACHE
		range_data_t temp = range;
#endif
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change, false, false);
#ifdef	RANGE_CACHE
		range = temp;
#endif
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
		if (same_range_flag && (target_addr & ~(4095) ) == ( (target_addr + target_size - 1) & ~(4095) ) )//Check if they are in the same mid page first.
			page_break_same_mid (begin_hit_before, begin_hit_after, end_hit_after);
		else if (same_range_flag && (target_addr & ~(4194303) ) == ( (target_addr + target_size - 1) & ~(4194303) ) )
			page_break_same_top (begin_hit_before, begin_hit_after, end_hit_after);//To prevent double counting for top break if start
		else {
			page_break (begin_hit_before, begin_hit_after);
			page_break (end_hit_before, end_hit_after);
		}
		
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::rm_read (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		bool same_range_flag = false;
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		if (beg_iter == end_iter && (beg_iter == wp.end() || addr_covered(target_addr, *beg_iter) == addr_covered(target_addr + target_size - 1, *end_iter) ) )
			same_range_flag = true;
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);

        // Removing a read watchpoint is an update, which would require loading the original data into the lookaside buffer & range cache
        general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_hit, trie.mid_hit, trie.bot_hit, true, true);
		
		rm_watchpoint (target_addr, target_size, WA_READ);
#ifdef	RANGE_CACHE
		range_data_t temp = range;
#endif
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change, false, false);
#ifdef	RANGE_CACHE
		range = temp;
#endif
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
		if (same_range_flag && (target_addr & ~(4095) ) == ( (target_addr + target_size - 1) & ~(4095) ) )//Check if they are in the same mid page first.
			page_break_same_mid (begin_hit_before, begin_hit_after, end_hit_after);
		else if (same_range_flag && (target_addr & ~(4194303) ) == ( (target_addr + target_size - 1) & ~(4194303) ) )
			page_break_same_top (begin_hit_before, begin_hit_after, end_hit_after);//To prevent double counting for top break if start
		else {
			page_break (begin_hit_before, begin_hit_after);
			page_break (end_hit_before, end_hit_after);
		}
		
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::rm_write (ADDRESS target_addr, ADDRESS target_size) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator beg_iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator end_iter;
		bool same_range_flag = false;
		PAGE_HIT begin_hit_before;
		PAGE_HIT end_hit_before;
		PAGE_HIT begin_hit_after;
		PAGE_HIT end_hit_after;

		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		if (beg_iter == end_iter && (beg_iter == wp.end() || addr_covered(target_addr, *beg_iter) == addr_covered(target_addr + target_size - 1, *end_iter) ) )
			same_range_flag = true;
		begin_hit_before = page_level (target_addr, beg_iter);
		end_hit_before = page_level (target_addr + target_size - 1, end_iter);

        // Removing a write watchpoint is an update, which would require loading the original data into the lookaside buffer & range cache
        general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_hit, trie.mid_hit, trie.bot_hit, true, true);
		
		rm_watchpoint (target_addr, target_size, WA_WRITE);
#ifdef	RANGE_CACHE
		range_data_t temp = range;
#endif
		general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_change, trie.mid_change, trie.bot_change, false, false);
#ifdef	RANGE_CACHE
		range = temp;
#endif
		
		beg_iter = search_address(target_addr, wp);
		end_iter = search_address(target_addr + target_size - 1, wp);
		begin_hit_after = page_level (target_addr, beg_iter);
		end_hit_after = page_level (target_addr + target_size - 1, end_iter);
		
		if (same_range_flag && (target_addr & ~(4095) ) == ( (target_addr + target_size - 1) & ~(4095) ) )//Check if they are in the same mid page first.
			page_break_same_mid (begin_hit_before, begin_hit_after, end_hit_after);
		else if (same_range_flag && (target_addr & ~(4194303) ) == ( (target_addr + target_size - 1) & ~(4194303) ) )
			page_break_same_top (begin_hit_before, begin_hit_after, end_hit_after);//To prevent double counting for top break if start
		else {
			page_break (begin_hit_before, begin_hit_after);
			page_break (end_hit_before, end_hit_after);
		}
		
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	bool WatchPoint<ADDRESS, FLAGS>::general_fault (ADDRESS target_addr, ADDRESS target_size, FLAGS target_flags, unsigned long long& top_page, unsigned long long& mid_page, unsigned long long& bot_page, bool lookaside, bool hit_miss_care) {
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
        watchpoint_t<ADDRESS, FLAGS> temp_val;
        temp_val.addr = target_addr;
        temp_val.size = target_size;
        bool did_hit = false;

        // If we're looking for a watchpoint for "anything", see if anything is in the WLB.        
        if (target_size == 0) {
			top_page++;
            if (lookaside) {
                if (wlb.size()) // SOMETHING is in the WLB... hit!
                    trie.wlb_hit_top++;
                else {
                    trie.wlb_miss_top++;
                }
            }
#ifdef RANGE_CACHE
            // TODO: We need to find a way to just "always hit" in the range cache.
            //Range_load(0, -1, hit_miss_care);
            //Range_cleanup();
#endif
			return false;
		}

        // Check the Lookaside buffer and update its LRU status.  See if we WLB hit.
        if (lookaside) {
            deque<watchpoint_t<ADDRESS, FLAGS> > guys_to_push;
            for (iter = wlb.begin(); iter != wlb.end();) {
                if(((iter->addr + iter->size) > target_addr) && (iter->addr < (target_addr + target_size))) {
                    did_hit = 1;
                    if (iter->size == 0) {
                        trie.wlb_hit_top++;
                        break;
                    }
                    guys_to_push.push_back(*iter);
                    iter = wlb.erase(iter);
                }
                else
                    iter++;
            }
            for (iter = guys_to_push.begin(); iter != guys_to_push.end(); iter++) {
                wlb.push_front(*iter);
            }
            guys_to_push.clear();
        }

		iter = search_address(target_addr, wp);
		
		if (iter != wp.end() && iter->size == 0) {
            //This is special for size = 0
            if (lookaside) {
                // If there's anything in the WLB, it must be the value pointing to "everything".
                if (wlb.size())
                    trie.wlb_hit_top++;
                else {
                    // Otherwise WLB must load the single value.  This was a miss.
                    temp_val.addr = 0;
                    temp_val.size = -1;
                    temp_val.flags = WA_READ|WA_WRITE;
                    wlb.push_front(temp_val);
                    trie.wlb_miss_top++;
                }
            }
			top_page++;
#ifdef RANGE_CACHE
			Range_load(0, -1, hit_miss_care);
            Range_cleanup();
#endif
			return (iter->flags & target_flags);
		}
		if (iter == wp.end() ) {
            // Not only was the value not in the watchpoint system, we're also at the end of the queue.
			if (iter != wp.begin() ) {
				iter--;
                temp_val.flags = WA_WRITE|WA_READ;
				if (iter->addr + iter->size -1 >= (target_addr & ~(4095) ) ) {
                    // Is the last watchpoint (we missed) on the same 4K page as our target?
                    if (lookaside) {
                        // Must use a 32-bit (16 watchpoint) LB entry.
                        temp_val.addr = target_addr & ~(15);
                        temp_val.size = 16;

                        if (did_hit)
                            trie.wlb_hit_bot++;
                        else
                            trie.wlb_miss_bot++;
                    }
#ifdef PAGE_TABLE
					if (lookaside)
						pagetable.access++;//This is the case that the page is marked as "watched" but the address is actually "unwatched".
#endif
                    bot_page++;
                }
				else if (iter->addr + iter->size - 1 >= (target_addr & ~(4194303) ) ) {
                    // Is the last watchpoint (we missed) instead on a 4M page with us?
                    if (lookaside) {
                        // Must use 4KB LB entry
                        temp_val.addr = target_addr & ~(4095);
                        temp_val.size = 4096;

                        if (did_hit)
                            trie.wlb_hit_mid++;
                        else
                            trie.wlb_miss_mid++;
                    }
					mid_page++;
                }
				else {
                    // Else it's somewhere in memory with us a long way off.
                    if (lookaside) {
                        // Must use 4MB LB entry
                        temp_val.addr = target_addr & ~(4194303);
                        temp_val.size = 4194304;

                        if (did_hit)
                            trie.wlb_hit_top++;
                        else
                            trie.wlb_miss_top++;
                    }
                    top_page++;
                }
                if (lookaside && !did_hit) {
                    // We're going to return below (obvious watchpoint miss) so we must update the WLB here.
                    if (wlb.size() > WLB_SIZE) // WLB full, remove theo ldest entry.
                        wlb.pop_back();
                    wlb.push_front(temp_val);
                }
#ifdef RANGE_CACHE
				Range_load(iter->addr + iter->size, -1, hit_miss_care);
#endif
			}
			else {
                // Empty watchpoint case.  We're both beginning and end.
                // Range cache should cover all of memory with "no watchpoint"
#ifdef RANGE_CACHE
				Range_load(0, -1, hit_miss_care);
#endif
                if (lookaside) {
                    // Lookaside agrees.  No watchpoints, so entry covers all memory.
                    if (!did_hit) {
                        temp_val.addr = 0;
                        temp_val.size = -1;
                        temp_val.flags = 0;

                        if (wlb.size() > WLB_SIZE)
                            wlb.pop_back();
                        wlb.push_front(temp_val);
                        trie.wlb_miss_top++;
                    }
                    else
                        trie.wlb_hit_top++;
                }
				top_page++;
			}
#ifdef RANGE_CACHE
            Range_cleanup();
#endif
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
#ifdef PAGE_TABLE
		ADDRESS start_page = 0;
		ADDRESS end_page = 0;
		if (lookaside) {
			if (iter == wp.begin() )
				end_page = 0;
			else {
				end_page = ( (iter-1)->addr + (iter-1)->size - 1) >> 12;
				if (end_page == target_addr >> 12)
					pagetable.access++;
			}
		}
#endif
		ADDRESS beg_addr;
		ADDRESS end_addr;
		
		while (iter != wp.end() && iter->addr <= target_addr + target_size - 1) {
#ifdef RANGE_CACHE
			if (!addr_covered (target_addr, *iter ) ) {
				if (iter == wp.begin() && iter->addr != 0) {
					end_addr = iter->addr - 1;

					Range_load (0, end_addr, hit_miss_care);
				}
				else if (iter != wp.begin() && (iter - 1)->addr + (iter - 1)->size != iter->addr) {
					beg_addr = (iter - 1)->addr + (iter - 1)->size;
					end_addr = iter->addr - 1;
					Range_load (beg_addr, end_addr, hit_miss_care);
				}
			}
			beg_addr = iter->addr;
			end_addr = iter->addr + iter->size - 1;
			Range_load(beg_addr, end_addr, hit_miss_care);
#endif

#ifdef PAGE_TABLE
			if (lookaside) {
				if (iter->addr < target_addr)
					start_page = target_addr >> 12;
				else
					start_page = iter->addr >> 12;
				if (start_page == end_page)
				//In case this wp shares the same page with the wp in front of it. We only need to count for once even both of them have parts within this page.
					pagetable.access--;
				if (target_addr + target_size - 1 < iter->addr + iter->size - 1)
					end_page = (target_addr + target_size - 1) >> 12;
				else
					end_page = (target_addr + target_size - 1) >> 12;
				pagetable.access += (end_page - start_page + 1);
				pagetable.wp_hit += (end_page - start_page + 1);
			}
#endif

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

#ifdef RANGE_CACHE
		if (iter != wp.begin() && !addr_covered (target_addr, (*(iter - 1) ) ) ) {
			beg_addr = (iter - 1)->addr + (iter - 1)->size;
			if (iter == wp.end() )
				end_addr = -1;
			else
				end_addr = iter->addr - 1;

			Range_load(beg_addr, end_addr, hit_miss_care);
		}
		else if (iter == wp.begin() ) {
			end_addr = iter->addr - 1;
			Range_load(0, end_addr, hit_miss_care);
		}
#endif

#ifdef PAGE_TABLE
		if (lookaside) {
			if (iter != wp.end() && !addr_covered (target_addr + target_size - 1, *(iter-1) ) ) {
				start_page = iter->addr >> 12;
				if (start_page <= ( (target_addr + target_size - 1) >> 12) && start_page != end_page)
					pagetable.access++;
			}
		}
#endif

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

        temp_val.flags = WA_WRITE|WA_READ;
		if (bot_level) {
            if (lookaside) {
                temp_val.addr = target_addr & ~(15);
                temp_val.size = 16;

                if (did_hit)
                    trie.wlb_hit_bot++;
                else
                    trie.wlb_miss_bot++;
            }
			bot_page++;
        }
		else if (mid_level) {
            if (lookaside) {
                temp_val.addr = target_addr & ~(4095);
                temp_val.size = 4096;

                if (did_hit)
                    trie.wlb_hit_mid++;
                else
                    trie.wlb_miss_mid++;
            }
			mid_page++;
        }
		else {
            if (lookaside) {
                temp_val.addr = target_addr & ~(4194303);
                temp_val.size = 4194304;

                if (did_hit)
                    trie.wlb_hit_top++;
                else
                    trie.wlb_miss_top++;
            }
			top_page++;
        }

        if (lookaside && !did_hit) {
            if (wlb.size() > WLB_SIZE)
                wlb.pop_back();
            wlb.push_front(temp_val);
        }
#ifdef RANGE_CACHE
        Range_cleanup();
#endif
		
		//	cout << "Exited general_fault" << endl;
		return wp_fault;
	}

	template <class ADDRESS, class FLAGS>
	bool WatchPoint<ADDRESS, FLAGS>::watch_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, (WA_READ | WA_WRITE), trie.top_hit, trie.mid_hit, trie.bot_hit, true, true) );
	}
	
	template <class ADDRESS, class FLAGS>
	bool WatchPoint<ADDRESS, FLAGS>::read_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, WA_READ, trie.top_hit, trie.mid_hit, trie.bot_hit, true, true) );
	}
	
	template <class ADDRESS, class FLAGS>
	bool WatchPoint<ADDRESS, FLAGS>::write_fault(ADDRESS target_addr, ADDRESS target_size) {
		return (general_fault (target_addr, target_size, WA_WRITE, trie.top_hit, trie.mid_hit, trie.bot_hit, true, true) );
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

#ifdef RANGE_CACHE    
	template <class ADDRESS, class FLAGS>
	range_data_t WatchPoint<ADDRESS, FLAGS>::get_range_data() {
		return range;
	}
#endif

#ifdef PAGE_TABLE
	template <class ADDRESS, class FLAGS>
	pagetable_data_t WatchPoint<ADDRESS, FLAGS>::get_pagetable_data() {
		return pagetable;
	}
#endif
	
	template <class ADDRESS, class FLAGS>
	typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator WatchPoint<ADDRESS, FLAGS>::Insert_wp (const watchpoint_t<ADDRESS, FLAGS>& insert_wp, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter) {
#ifdef RANGE_CACHE
		ADDRESS		start_addr;
		ADDRESS		end_addr;
		range_t<ADDRESS>	insert_range;
		typename deque< range_t<ADDRESS> >::iterator	range_cache_iter;
		
		end_addr = iter->addr -1;
		
		if (iter == wp.begin() )
			start_addr = 0;
		else
			start_addr = (iter - 1)->addr + (iter - 1)->size;
		
		range_cache_iter = Range_load(start_addr, end_addr, false);//as insert must be insert into some kinds of blank, so there must be an unwatched range.
		range_cache_iter->start_addr = insert_wp.addr;//Change the size of the unwatched range, and make it as watched range.
		range_cache_iter->end_addr = insert_wp.addr + insert_wp.size - 1;
        range_cache_iter->dirty = true;
		if (start_addr != insert_wp.addr) {//split front
			insert_range.start_addr = start_addr;
			insert_range.end_addr = insert_wp.addr - 1;
            insert_range.dirty = true;
			range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
			range.cur_range_num++;
		}
		if (end_addr != insert_wp.addr + insert_wp.size - 1) {//split back
			insert_range.start_addr = insert_wp.addr + insert_wp.size;
			insert_range.end_addr = end_addr;
            insert_range.dirty = true;
			range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
			range.cur_range_num++;
		}
#endif
		iter = wp.insert(iter, insert_wp);
		return iter;
	}
	
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::Modify_wp (const watchpoint_t<ADDRESS, FLAGS>& modify_t, typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter) {
#ifdef RANGE_CACHE
		ADDRESS		start_addr;
		ADDRESS		end_addr;
		range_t<ADDRESS>	insert_range;
        bool delay_insert = false;
		typename deque< range_t<ADDRESS> >::iterator	range_cache_iter;
		
		if (modify_t.addr != iter->addr) {
			if (iter != wp.begin() )
				start_addr = (iter - 1)->addr + (iter - 1)->size;
			else
				start_addr = 0;
			if (start_addr == iter->addr) {
                // There are no non-watched ranges between the two watchpoint regions, and thus their flags are different.
                // Because of this, we need to add a new region into the system that contains our "shrink" of the beginning if iter.
				insert_range.end_addr = modify_t.addr - 1;
				insert_range.start_addr = start_addr;
                insert_range.dirty = true;
				//range_cache_iter = range_cache.begin();
				//range_cache.insert(range_cache_iter, insert_range);
                // Delay this insertion so that the Range_load() below so it doesn't detect this range instead!
                delay_insert = true;
				range.cur_range_num++;
			}
			else { 
                // There are non-watched ranges bewteen the two watchpoints.
				end_addr = iter->addr - 1;
				range_cache_iter = Range_load(start_addr, end_addr, false); // Find that non-watched range
				if (modify_t.addr == start_addr) {
                    // If the address it starts on is the modified range's beginning, it goes away.
					range_cache.erase(range_cache_iter);
					range.cur_range_num--;
				}
				else // else it just changes size.
					range_cache_iter->end_addr = modify_t.addr - 1;
			}
		}
		
		start_addr = iter->addr;
		end_addr = iter->addr + iter->size - 1;
		range_cache_iter = Range_load(start_addr, end_addr, false);
		range_cache_iter->start_addr = modify_t.addr;
		range_cache_iter->end_addr = modify_t.addr + modify_t.size - 1;
        range_cache_iter->dirty = true;
        if (delay_insert) {
            range_cache_iter = range_cache.begin();
            range_cache.insert(range_cache_iter, insert_range);
        }
		
		if (iter->addr + iter->size != modify_t.addr + modify_t.size) {
            // End also changed.  We'll need to modify the range after us.
			if (iter != (wp.end() - 1) )
				end_addr = -1;
			else
				end_addr = (iter + 1)->addr - 1;
			if (end_addr == iter->addr + iter->size - 1) {
                // The old end touched the guy after him, so no watchpoints in between them.  We need to insert a new range between the two.
				insert_range.start_addr = modify_t.addr + modify_t.size;
				insert_range.end_addr = end_addr;
                insert_range.dirty = true;
				range_cache_iter = range_cache.begin();
				range_cache.insert(range_cache_iter, insert_range);
				range.cur_range_num++;
			}
			else {
                // There were non-watched ranges between the two watchpoints, so we just need to move that range's beginning.
				start_addr = iter->addr + iter->size;
				range_cache_iter = Range_load(start_addr, end_addr, false);
				if (modify_t.addr + modify_t.size - 1 == end_addr) {
                    // If the address it startedon is on is the modified range's end, it goes away.
					range_cache.erase(range_cache_iter);
					range.cur_range_num--;
				}
				else { // Else it just changes size.
					range_cache_iter->start_addr = modify_t.addr + modify_t.size;
                    range_cache_iter->dirty = true;
                }
			}
		}
#endif
		iter->addr = modify_t.addr;
		iter->size = modify_t.size;
		iter->flags = modify_t.flags;
		return;
	}
	
	template <class ADDRESS, class FLAGS>	
	void WatchPoint<ADDRESS, FLAGS>::Push_back_wp (const watchpoint_t<ADDRESS, FLAGS>& insert_wp) {
#ifdef RANGE_CACHE
		ADDRESS		start_addr;
		range_t<ADDRESS>	insert_range;
		typename deque< range_t<ADDRESS> >::iterator	range_cache_iter;
		
		if (wp.size() == 0) {// There is no watchpoint that covers the entire memory
			if (insert_wp.size == 0) {
				range.hit++;
				wp.push_back(insert_wp);
				return;
			}
			Range_load(0, -1, false);//This is used just to increment hit.
			range_cache_iter = range_cache.begin();//So the only range currently in range_cache is [0, -1]
            if(range_cache_iter == range_cache.end()) {
                fprintf(stderr, "Incorrect range cache iter in Push_back_wp\n");
                exit(-1);
            }
			range_cache_iter->start_addr = insert_wp.addr;
			range_cache_iter->end_addr = insert_wp.addr + insert_wp.size - 1;
            range_cache_iter->dirty = true;
			if (insert_wp.addr != 0) {
				insert_range.start_addr = 0;
				insert_range.end_addr = insert_wp.addr - 1;
                insert_range.dirty = true;
				range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
				range.cur_range_num++;
			}
			if (insert_wp.addr + insert_wp.size != 0) {
				insert_range.start_addr = insert_wp.addr + insert_wp.size - 1;
				insert_range.end_addr = -1;
                insert_range.dirty = true;
				range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
				range.cur_range_num++;
			}
		}
		else {
			start_addr = (wp.end() - 1)->addr - 1;//When called push_back, there must be space at the end of "memory". So start_addr != -1
			range_cache_iter = Range_load(start_addr, -1, false);
			range_cache_iter->start_addr = insert_wp.addr;
			range_cache_iter->end_addr = insert_wp.addr + insert_wp.size - 1;
            range_cache_iter->dirty = true;
			if (insert_wp.addr != start_addr) {
				insert_range.start_addr = start_addr;
				insert_range.end_addr = insert_wp.addr - 1;
                insert_range.dirty = true;
				range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
				range.cur_range_num++;
			}
			if (insert_wp.addr + insert_wp.size != 0) {
				insert_range.start_addr = insert_wp.addr + insert_wp.size;
				insert_range.end_addr = -1;
                insert_range.dirty = true;
				range_cache_iter = range_cache.insert(range_cache_iter, insert_range);
				range.cur_range_num++;
			}
		}
#endif
		wp.push_back(insert_wp);
		return;
	}
	
	template <class ADDRESS, class FLAGS>
	typename deque< watchpoint_t<ADDRESS, FLAGS> >::iterator WatchPoint<ADDRESS, FLAGS>::Erase_wp (typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter) {
#ifdef RANGE_CACHE
		ADDRESS		start_addr;
		ADDRESS		end_addr;
		ADDRESS		ulti_start_addr;
		ADDRESS		ulti_end_addr;
		range_t<ADDRESS>	insert_range;
		typename deque< range_t<ADDRESS> >::iterator	range_cache_iter;
		
		//Front Blank
		end_addr = iter->addr - 1;
		
		if (iter == wp.begin())
			start_addr = 0;
		else
			start_addr = (iter - 1)->addr + (iter - 1)->size;
		ulti_start_addr = start_addr;

		if (end_addr >= start_addr) {
			range_cache_iter = Range_load(start_addr, end_addr, false);
			range_cache_iter = range_cache.erase(range_cache_iter);
			range.cur_range_num--;
		}
		//Middle
		start_addr = iter->addr;
		end_addr = iter->addr + iter->size - 1;
		range_cache_iter = Range_load(start_addr, end_addr, false);
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
			range_cache_iter = Range_load(start_addr, end_addr, false);
			range_cache_iter = range_cache.erase(range_cache_iter);
			range.cur_range_num--;
		}
		//Add the new merged unwatched range.
		insert_range.start_addr = ulti_start_addr;
		insert_range.end_addr = ulti_end_addr;
        insert_range.dirty = true;
		range_cache.insert(range_cache_iter, insert_range);
		range.cur_range_num++;
#endif
		iter = wp.erase(iter);
		return iter;
	}
	
#ifdef RANGE_CACHE
	template <class ADDRESS, class FLAGS>
	typename deque< range_t<ADDRESS> >::iterator WatchPoint<ADDRESS, FLAGS>::Range_load (ADDRESS start_addr, ADDRESS end_addr, bool hit_miss_care) {
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
            if (hit_miss_care)
			    range.hit++;
			insert_range = *iter;
			range_cache.erase(iter);
		}
		else {
			insert_range.start_addr = start_addr;
			insert_range.end_addr = end_addr;
            insert_range.dirty = false;
            if (hit_miss_care)
			    range.miss++;
		}
		iter = range_cache.begin();
		iter = range_cache.insert(iter, insert_range);
		return iter;
	}
#endif

#ifdef RANGE_CACHE
	template <class ADDRESS, class FLAGS>
	void WatchPoint<ADDRESS, FLAGS>::Range_cleanup() {
		if (range_cache.size() > RANGE_CACHE_SIZE) {
			typename deque< range_t<ADDRESS> >::iterator iter;
            range.kick += range_cache.size() - RANGE_CACHE_SIZE;
            bool found_dirty = false;
			for (int i = range_cache.size() - RANGE_CACHE_SIZE; i > 0; i--) {
				iter = range_cache.end() - 1;
                if(iter->dirty) {
                    found_dirty = true;
                    range.kick++;
                }
				range_cache.erase(iter);
			}
            if (found_dirty) {
                for (iter = range_cache.begin(); iter != range_cache.end(); iter++) {
                    if (iter->dirty) {
                        iter->dirty = false;
                        range.kick++;
                    }
                }
                range.dirty_kick++;
            }
		}
		range.changes++;
        range.total_cur_range_num += range.cur_range_num;
		
		if (range.cur_range_num > range.max_range_num)
			range.max_range_num = range.cur_range_num;
		return;
	}
#endif

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
        wp.clear();
		wp.resize((int)parameter.wp.size());
        unsigned int i;
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
        unsigned int i;
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
		watchpoint_t<ADDRESS, FLAGS> insert_t = {0, 0, 0};

        // This is special for size=0.  As it clears all the current
        // watchpoints and adds the whole system as watched.
        if (target_size == 0) {
            wp.clear();
            insert_t.flags = target_flags;
            wp.push_back(insert_t);
            return;
        }

		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator start_iter;//This one is used only for merging the front wp nodes.
		iter = search_address(target_addr, wp);
		
        if (iter != wp.end() && iter->size == 0) { // This is special for size = 0.  It would then check for flag inclusion and split it if necessary.
            // If the flags between the all-memory iterator and the newly-added
            // watchpoint are different, we need to either update the old WP,
            // or split the all-memory WP into multiple pieces.
            if (!flag_inclusion(target_flags, iter->flags) ) {
                if (target_addr == 0) {
                    // If the new region is at the beginning of memory, split WP in two
                    insert_t.addr = target_addr + target_size;
                    insert_t.size = 0 - insert_t.addr;
                    insert_t.flags = iter->flags;
                    iter->size = target_size;
                    iter->flags = target_flags | iter->flags;
                    wp.push_back(insert_t);
                }
                else if (target_addr + target_size == 0) {
                    // If the new region is at the end of memory, split WP in two.
                    insert_t.addr = target_addr;
                    insert_t.size = target_size;
                    insert_t.flags = iter->flags | target_flags;
                    wp.push_back(insert_t);
                    iter->size = target_addr;
                }
                else {
                    // If the new region is in the middle, split WP into three.
                    insert_t.addr = 0;
                    insert_t.size = target_addr;
                    insert_t.flags = iter->flags;
                    iter->addr = target_addr;
                    iter->size = target_size;
                    iter->flags = target_flags | iter->flags;
                    wp.insert(iter, insert_t);
                    insert_t.addr = target_addr + target_size;
                    insert_t.size = 0 - insert_t.addr;
                    insert_t.flags = iter->flags;
                    wp.push_back(insert_t);
                }
            }
            return;
        }

		if (iter == wp.end() ) {
            // Not found, and the last WP doesn't even reac our address.
			if (iter != wp.begin() ) {
                // Check if there is some WP before iter; if there is, we
                // potentially need to merge them into one.
				start_iter = iter - 1;
				if (start_iter->addr + start_iter->size == target_addr && start_iter->flags == target_flags) {
                    //Merge condition.
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
            // Our current range is covered by a watchpoint
			if (iter->addr == target_addr) {
                //We will only need to consider merging if this is true.
				if (iter != wp.begin() ) {
					start_iter = iter - 1;
					if(start_iter->addr + start_iter->size == target_addr && start_iter->flags == (iter->flags | target_flags) ) {
                        //We will have to merge the two node.
                        if (iter->addr + iter->size - 1 > target_addr + target_size - 1 && !flag_inclusion (target_flags, iter->flags) ) {
                            // If the thing we actually hit encompasses this
                            // watchpoint, we must split it.
							start_iter->size += target_size;
							iter->addr = target_addr + target_size;
							iter->size -= target_size;
							return;
						}
                        // If the current insertion is bigger than what it hit
                        // Just delete the old two guys and replace it with a
                        // large range.  If you have [start_iter][iter][third]
                        // and you added range overflows into a third (or more)
                        // the iterating code below covers it.
                        insert_t.addr = start_iter->addr;
						insert_t.size = start_iter->size + iter->size;
                        insert_t.flags = start_iter->flags;
						iter = wp.erase(start_iter);
						iter = wp.erase(iter);
					}
					else {
                        // They don't touch, so theres no merging to take place
						insert_t.addr = iter->addr;
						insert_t.flags = iter->flags | target_flags;
						if (iter->addr + iter->size - 1 > target_addr + target_size - 1 && !flag_inclusion (target_flags, iter->flags) ) {
                            // We will need to split the range into two if the
                            // iter encompasses the insertion & flags differ.
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
                    // Nobody before us.  We are completely encompassed, so
                    // split into 2.
					iter->size = iter->size - target_size;
					iter->addr = target_addr + target_size;
					insert_t.addr = target_addr;
					insert_t.size = target_size;
					insert_t.flags = target_flags | iter->flags;
					wp.insert(iter, insert_t);
					return;
				}
				else {
                    // Just 'extend' the range of the old guy below.  Get rid
                    // of him here.
					insert_t.addr = iter->addr;
					insert_t.size = iter->size;
					insert_t.flags = iter->flags | target_flags;
					iter= wp.erase(iter);
				}
			}
			else {
                // We aren't aligned with the front of our hit range.
                // We will need to consider splitting.
				if (flag_inclusion (target_flags, iter->flags) ) {
                    //if the flag is included
					//then just mark the node as "to be inserted"
					insert_t.addr = iter->addr;
					insert_t.size = iter->size;
					insert_t.flags = iter->flags;
					iter = wp.erase(iter);
                    //!!As this watchpoint node is erased, iter automatically increments by 1.
				}
				else {
                    //if the flag is not included, we then need to split the watchpoint
					if (iter->addr + iter->size - 1 > target_addr + target_size - 1) {
                        // Completely contained within the iter watchpoint,
                        // so split into 3.
                        // Insert third partition.
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
                    // This guy flies off the end of the current watchpoint.
                    // Just shrink the beginning and add WP at the end.
					insert_t.size = iter->addr + iter->size - target_addr;
					insert_t.addr = target_addr;
					insert_t.flags = target_flags | iter->flags;
					iter->size = target_addr - iter->addr;//split the watchpoint by modifying iter's length
					iter++;//increment to the next node
				}
			}
		}
		else {
            //The target_addr is not covered at all by the iterator.
			if (iter != wp.begin()) {
                //Check to see if the WP to be added should be merged with a WP before it.
				start_iter = iter - 1;
				if (start_iter->addr + start_iter->size == target_addr && start_iter->flags == target_flags) {
                    // Merge them!
					insert_t.addr = start_iter->addr;
					if (iter->addr <= target_addr + target_size - 1) // Need to potentially merge end with iter below
						insert_t.size = iter->addr - insert_t.addr;
					else
						insert_t.size = target_size + target_addr - insert_t.addr; // Add insert onto the end
					insert_t.flags = target_flags;
					iter = wp.erase(start_iter); // Erase old guy
				}
				else {
                    // Else we will have just a regular insertion.
					insert_t.addr = target_addr;
					if (iter->addr <= target_addr + target_size - 1) // Need to potentially merge end with iter below
						insert_t.size = iter->addr - target_addr;
					else
						insert_t.size = target_size;
					insert_t.flags = target_flags;
				}
			}
			else {
                // This should become the first watchpoint in the system.
                // May need to merge the end, but not the beginning.
				insert_t.addr = target_addr;
				if (iter->addr <= target_addr + target_size - 1)
					insert_t.size = iter->addr - target_addr;
				else
					insert_t.size = target_size;
				insert_t.flags = target_flags;
			}
		}
		
		
		//Iterating part--If we are inserting something that covers multiple old watchpoints,
        //we must appropriately merge them all together.
		while (iter != wp.end() && iter->addr + iter->size - 1 <= target_addr + target_size - 1) {
            // We only need to perform this while our insertion guy reaches past the end of each iterator
			if (iter->addr != insert_t.addr + insert_t.size) {
                //If there is some blank between the two nodes.
				if (insert_t.flags == target_flags) {
                    //If the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = iter->addr - insert_t.addr;
                }
				else {
                    //if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
					iter = wp.insert(iter, insert_t);
					iter++;
					insert_t.addr = insert_t.addr + insert_t.size;
					insert_t.size = iter->addr - insert_t.addr;
					insert_t.flags = target_flags;
				}
			}
			if (insert_t.flags == (iter->flags | target_flags) ) {
                //then we will need to merge the two node, by just enlarging the insert_t size
				insert_t.size = insert_t.size + iter->size;
            }
			else {
                //if not, then we will first insert insert_t into wp, and update insert_t.
				iter = wp.insert(iter, insert_t);
				iter++;
				insert_t.addr = iter->addr;
				insert_t.size = iter->size;
				insert_t.flags = iter->flags | target_flags;
			}
			iter = wp.erase(iter);
            //anyway, we will need to delete the wp we walked through, since we will add them back in the future(Which is also in this code).
		}
		
		//Ending part - Insert the last large watchpoint.
		if (iter == wp.end() || !addr_covered( (target_addr + target_size), (*iter) ) ) {
            //If it's the end of the deque or the end+1 address is not covered by any wp
			//Then we simply add the watchpoint on it.
			if (target_addr + target_size - 1 > insert_t.addr + insert_t.size - 1) {
                //If there is still some blank between insert_t and the end of the adding address.
				if (insert_t.flags == target_flags) {
                    //if the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = target_addr + target_size - insert_t.addr;
				}
				else {
                    //if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
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
		else {
            //Then the end+1 address is covered by some wp. Then we'll need to test whether the flags are the same.
			//First check if there is any blank between the two nodes.
			if (iter->addr != insert_t.addr + insert_t.size) {
                //If there is some blank between the two nodes.
				if (insert_t.flags == target_flags) {
                    //if the insert node's flag matches the output node, then we just enlarge the insert node.
					insert_t.size = iter->addr - insert_t.addr;
                }
				else {
                    //if the flag doesn't match, then we will need to write the "insert" into wp and refresh "insert"
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
			else if (iter->addr <= target_addr + target_size - 1) {
                //If the ending address is not covered, no need to split. (Merging is done by above)
				if (flag_inclusion (target_flags, iter->flags) ) {
                    //if the target_flag is included by iter, then we just left it as before
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
		if (target_size == 0) { // This is special for size = 0;
            wp.clear();
			return;
        }
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator iter;
		typename deque<watchpoint_t<ADDRESS, FLAGS> >::iterator previous_iter;
		
        //starting part
		iter = search_address(target_addr, wp);

        if (iter != wp.end() && iter->size == 0 && (iter->flags & target_flags) ) {
            // This is special for size = 0.  We're deleting the "all memory
            // covered" watchpoint.
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
            // Check to see if the iterator needs split.
            // If it starts at least one addres before the guy we're adding,
            // we must split.
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
        if (iter != wp.end() && iter->size == 0)
            return (iter->flags & target_flags);
		if (iter == wp.end() )
			return false;
		while (iter != wp.end() && iter->addr <= target_addr + target_size - 1) {
			if (iter->flags & target_flags)
				return true;
			iter++;
            if(iter == wp.end()) {
                break;
            }
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

