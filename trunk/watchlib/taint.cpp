//someday this tool will track tainted memory throughout an application

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syscall.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include "pin.H"
#include "xed-interface.h"
#include "xed-category-enum.h"
#include "xed-iclass-enum.h"
#include "auto_wp.h"

using std::deque;
using Hongyi_WatchPoint::WatchPoint;
using Hongyi_WatchPoint::trie_data_t;
using Hongyi_WatchPoint::MEM_WatchPoint;
//My own data
struct wp_data_t
{
	WatchPoint<ADDRINT, UINT32> wp;
};

wp_data_t taint_store;

trie_data_t trie_total;
//My own data
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "taint.out", "specify output file name");

#define REGS_AVAIL 225
#define REG_READ  0x01
#define REG_WRITE 0x02
#define MAX_FD 1024
//list of file descriptors that represent non-trusted sources
//lot's of assumptions here for 32bit-standard install Linux

//static char    shady_fd[MAX_FD];
static char    global_regs[REGS_AVAIL]; //each register used (what's the max?)

#define PAGE_MASK 0xFFF
#define PAGE_SHIFT 12
#define PAGE_SIZE 4096
#define MAX_PAGES 1048576
#define ONE_BYTE_TAINTED  0xFF
#define TWO_BYTE_TAINTED  0xFFFF
#define FOUR_BYTE_TAINTED 0xFFFFFFFF

#define IMMEDIATE_VAL 0

static void*   pages[MAX_PAGES];    //number of 4 KB pages in a 32 bit address space
static char ZERO_PAGE[PAGE_SIZE];
static void* ZERO_PAGE_LOC = ZERO_PAGE;

#define REG_TAINTED 1;

FILE* logfd;
FILE* errfd;
FILE* snpfd;

/*Important EFLAGS for CMOV */
#define CF_MASK 0x01
#define PF_MASK 0x04
#define ZF_MASK 0x40
#define SF_MASK 0x80
#define OF_MASK 0x800

#define CMOV_GENERIC 1
#define CMOVBE       2
#define CMOVL        3
#define CMOVNL       4
#define CMOVLE       5
#define CMOVNLE      6

//mirror linux/net.h for user-land
#define SYS_SOCKET	1		/* sys_socket(2)		*/
#define SYS_BIND	2		/* sys_bind(2)			*/
#define SYS_CONNECT	3		/* sys_connect(2)		*/
#define SYS_LISTEN	4		/* sys_listen(2)		*/
#define SYS_ACCEPT	5		/* sys_accept(2)		*/
#define SYS_GETSOCKNAME	6		/* sys_getsockname(2)		*/
#define SYS_GETPEERNAME	7		/* sys_getpeername(2)		*/
#define SYS_SOCKETPAIR	8		/* sys_socketpair(2)		*/
#define SYS_SEND	9		/* sys_send(2)			*/
#define SYS_RECV	10		/* sys_recv(2)			*/
#define SYS_SENDTO	11		/* sys_sendto(2)		*/
#define SYS_RECVFROM	12		/* sys_recvfrom(2)		*/
#define SYS_SHUTDOWN	13		/* sys_shutdown(2)		*/
#define SYS_SETSOCKOPT	14		/* sys_setsockopt(2)		*/
#define SYS_GETSOCKOPT	15		/* sys_getsockopt(2)		*/
#define SYS_SENDMSG	16		/* sys_sendmsg(2)		*/
#define SYS_RECVMSG	17		/* sys_recvmsg(2)		*/

//we can count the different types of instructions
#ifdef INS_COUNT
UINT32 total_ins = 0;
UINT32 syscall_ins = 0;
UINT32 mov_ins = 0;
UINT32 cmov_ins = 0;
UINT32 push_ins = 0;
UINT32 pop_ins = 0;
UINT32 xchg_ins = 0;
UINT32 arith_ins = 0;
UINT32 xadd_ins = 0;
UINT32 call_ins = 0;
UINT32 string_ins = 0;

void Fini(INT32 code, void *v);

void inc_cnt(UINT32 itm)
{
    UINT32* cnt = (UINT32*) itm;
    total_ins++;
//    fprintf(stderr, "itm val is 0x%08x cnt is 0x%08x \n", itm, (u_int) cnt);
    (*cnt)++;
//    fprintf(stderr, "inc inst at 0x%08x cnt is %u total is %u\n", (u_int)cnt,
//	    *cnt, total_ins);
}

void print_cnt()
{
#ifdef INS_COUNT_LOG
    fprintf(logfd, "Total instrumented instructions \t%u\n", total_ins);
    fprintf(logfd, "Total syscall %u\t %% %u\n", syscall_ins, syscall_ins/total_ins);
    fprintf(logfd, "Total mov_ins %u\t %% %u\n", mov_ins, mov_ins/total_ins);
    fprintf(logfd, "Total cmov_ins %u\t %% %u\n", cmov_ins, cmov_ins/total_ins);
    fprintf(logfd, "Total push_ins %u\t %% %u\n", push_ins, push_ins/total_ins);
    fprintf(logfd, "Total pop_ins %u\t %% %u\n",  pop_ins, pop_ins/total_ins);
    fprintf(logfd, "Total xchg_ins %u\t %% %u\n", xchg_ins, xchg_ins/total_ins);
    fprintf(logfd, "Total arith_ins %u\t %% %u\n", arith_ins, arith_ins/total_ins);
    fprintf(logfd, "Total xadd_ins %u\t %% %u\n", xadd_ins, xadd_ins/total_ins);
    fprintf(logfd, "Total call_ins %u\t %% %u\n", call_ins, call_ins/total_ins);
    fprintf(logfd, "Total string_ins %u\t %% %u\n", string_ins, string_ins/total_ins);
#else
    fprintf(stderr, "Total instrumented instructions \t%u\n", total_ins);
    fprintf(stderr, "Total syscall %u\t\t %% %.3f\n", syscall_ins, ((1.0* syscall_ins)/total_ins) * 100);
    fprintf(stderr, "Total mov_ins %u\t\t %% %.3f\n", mov_ins, ((1.0*mov_ins)/total_ins) * 100);
    fprintf(stderr, "Total cmov_ins %u\t\t %% %.3f\n", cmov_ins, ((1.0*cmov_ins)/total_ins)*100);
    fprintf(stderr, "Total push_ins %u\t\t %% %.3f\n", push_ins, ((1.0*push_ins)/total_ins)*100);
    fprintf(stderr, "Total pop_ins %u\t\t %% %.3f\n",  pop_ins, ((1.0*pop_ins)/total_ins)*100);
    fprintf(stderr, "Total xchg_ins %u\t\t %% %.3f\n", xchg_ins, ((1.0*xchg_ins)/total_ins)*100);
    fprintf(stderr, "Total arith_ins %u\t %% %.3f\n", arith_ins, ((1.0*arith_ins)/total_ins)*100);
    fprintf(stderr, "Total xadd_ins %u\t\t %% %.3f\n", xadd_ins, ((1.0*xadd_ins)/total_ins)*100);
    fprintf(stderr, "Total call_ins %u\t\t %% %.3f\n", call_ins, ((1.0*call_ins)/total_ins)*100);
    fprintf(stderr, "Total string_ins %u\t\t %% %.3f\n", string_ins, ((1.0*string_ins)/total_ins)*100);
#endif
}
#endif

#define DPRINT(...)
//#define DPRINT fprintf
static UINT32 slowpath = 0;
static UINT32 reallyslow = 0;

static inline BOOL is_valid_reg(REG reg)
{
  if(REG_is_gr32(reg) || REG_is_gr16(reg) || REG_is_gr8(reg) ||
       REG_is_mm(reg) || REG_is_xmm(reg)) {
	return 1;
    }

    return 0;
}

#if 0

static int valid_fd(int fd)
{
    if(fd < 0 || fd > 1024) {
	fprintf(stderr, "ERROR passed bogus fd value of %d\n", fd);
	return 0;
    }

    return 1;
}

static int add_shady_fd(int fd) {
    
    if(!valid_fd(fd)) return -1;
    DPRINT(logfd, "adding shady fd!!\n");
    if(shady_fd[fd]) {
	fprintf(stderr, "trying to add shady fd when it already exists?\n");
	return -1;
    }

    shady_fd[fd] = 1;
    return 0;
}

static int del_shady_fd(int fd) {

    if(!valid_fd(fd)) return -1;

    if(!shady_fd[fd]) {
	fprintf(stderr, "trying to delete shady fd that doesn't exist?\n");
	return -1;
    }

    shady_fd[fd] = 0;
    return 0;
}


static int is_shady_fd(int fd)
{
    if(!valid_fd(fd)) return -1;

    return shady_fd[fd];
}
#endif

/*we need to write some taint into a page
  so make sure we're not writing to the zero page

can we rid ourselves if this "if" with conditional analysis?
*/
static inline char* get_real_page(UINT32 pageindex)
{
    if(pages[pageindex] == ZERO_PAGE_LOC) {
	DPRINT(logfd, "allocing new page for pageindex %u\n", pageindex);
	pages[pageindex] = new char[4096];
	memset(pages[pageindex], 0, PAGE_SIZE);
    }
    return (char*) pages[pageindex];
}

void taint_addr_generic(u_int memloc, u_int written)
{
    UINT32 pageindex = (memloc >> PAGE_SHIFT);
    UINT32 loc = memloc & PAGE_MASK;
    UINT32 space = PAGE_SIZE - loc;
    char* page;
    UINT32 len;

    slowpath++;

    taint_store.wp.add_read_wp(memloc, written);

    while(written) {
	page = get_real_page(pageindex);

	loc = memloc & PAGE_MASK;
	space = PAGE_SIZE - loc;
	DPRINT(stderr, "loc within page is %u space is %u written is %u\n", loc, space, written);
	if(written > space) {
	    len = space;
	} else {
	    len = written;
	}
	//taint the bytes read in for this page
	page += loc;
	memset(page, 0xFF, len);
	DPRINT(logfd, "tainting page addr 0x%x len %u address 0x%08x page index is %u\n", (u_int)page, len, memloc, pageindex);
	written = written - len;
	memloc += len;
	pageindex++;
    }
}

void taint_addr_1byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page;
    slowpath++;

    taint_store.wp.add_read_wp(addr, 1);
    
    page = get_real_page(pageindex);
    page[offset] = ONE_BYTE_TAINTED;
    DPRINT(logfd, "tainting 1 byte at addr 0x%08x \n", addr);
}

void taint_addr_2byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page;
    UINT16* location;
    slowpath++;

    taint_store.wp.add_read_wp(addr, 2);
    
    page = get_real_page(pageindex);
    page += offset;
    location = (UINT16*) page;
    DPRINT(logfd, "tainting 2 bytes at addr 0x%08x \n", addr);
    *location = TWO_BYTE_TAINTED;
}

void taint_addr_4byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page;
    UINT32* location;
    slowpath++;

    taint_store.wp.add_read_wp(addr, 4);

    page = get_real_page(pageindex);
    page += offset;
    location = (UINT32*) page;
    DPRINT(logfd, "tainting 4 bytes at addr 0x%08x \n", addr);
    *location = FOUR_BYTE_TAINTED;
}    
    
void clear_addr_generic(UINT32 addr, UINT32 written)
{

    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];
    fprintf(logfd, "executing generic fcn!\n");
    slowpath++;
    taint_store.wp.rm_watch(addr, written);
    page += offset;
    memset(page, 0, written);
}

void clear_addr_1byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];    
    taint_store.wp.rm_watch(addr, 1);
    page[offset] = 0;
}

void clear_addr_2byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];   
    UINT16* location;
    taint_store.wp.rm_watch(addr, 2);

    page += offset;
    location = (UINT16*) page;

    *location = 0;
}    

void clear_addr_4byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];   
    UINT32* location;
    taint_store.wp.rm_watch(addr, 4);
    
    page += offset;
    location = (UINT32*) page;
    
    *location = 0;
}    

void clear_addr_8byte(UINT32 addr)
{
    UINT32 pageindex;
    UINT32 offset;
    char* page;
    UINT32* location;
    
    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = (char*) pages[pageindex];   
    page += offset;
    location = (UINT32*) page;
    taint_store.wp.rm_watch(addr, 8);

    addr += 4;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = (char*) pages[pageindex];   
    page += offset;
    location = (UINT32*) page;
    
    *location = 0;
}

void clear_addr_16byte(UINT32 addr)
{
    UINT32 pageindex;
    UINT32 offset;
    char* page;
    UINT32* location;
    
    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = (char*) pages[pageindex];   
    page += offset;
    location = (UINT32*) page;
    taint_store.wp.rm_watch(addr, 16);

    addr += 4;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = (char*) pages[pageindex];   
    page += offset;
    location = (UINT32*) page;

    addr += 4;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = (char*) pages[pageindex];   
    page += offset;
    location = (UINT32*) page;

    addr += 4;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = (char*) pages[pageindex];   
    page += offset;
    location = (UINT32*) page;
    
    *location = 0;
}

/*when copying taint values from a memory address being read
  we need to also copy the taint values of the registers
  used to calculate the effective address read
*/
void cp_taint_mem2reg_1byte(UINT32 addr, REG reg)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];        
    int result;
    slowpath++;
    taint_store.wp.read_fault(addr, 1);
    result = (page[offset] > 0);
    global_regs[reg] = result;
}

void cp_taint_mem2reg_2byte(UINT32 addr, REG reg)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];        
    UINT16* location;
    int result;
    slowpath++;
    page += offset;
    location = (UINT16*) page;

    result = (*location > 0);
    taint_store.wp.read_fault(addr, 2);
#if TAINT_DEBUG
    if(result > 0) {
	fprintf(logfd, "cp_taint_mem2reg: ztainting register %d addr 0x%08x value 0x%08x\n",
		reg, addr, *location);
    }
#endif
    global_regs[reg] = result;
}

void cp_taint_mem2reg_4byte(UINT32 addr, REG reg)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];        
    UINT32* location;
    int result;
    slowpath++;
    page += offset;

    location = (UINT32*) page;

    result = (*location > 0);
    taint_store.wp.read_fault(addr, 4);
#if TAINT_DEBUG
    if(result > 0) {
	fprintf(logfd, "cp taint mem2reg: tainting register %d addr 0x%08x value 0x%08x base val %d index val %d\n",
		reg, addr, *location, global_regs[base], global_regs[index]);
    }
#endif
    global_regs[reg] = result;
}

/*watch for unaligned large reads*/
void cp_taint_mem2reg_8byte(UINT32 addr, REG reg)
{
    UINT32 pageindex;
    UINT32 offset;
    char* page;
    UINT32* location;
    int result;
    slowpath++;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = (char*) pages[pageindex];        
    page += offset;
    location = (UINT32*) page;
    result = (*location > 0);

    addr += 4;
    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = (char*) pages[pageindex];        
    page += offset;
    location = (UINT32*) page;
    result |= (*location > 0);
    taint_store.wp.read_fault(addr, 8);

#if TAINT_DEBUG
    if(result > 0) {
	fprintf(logfd, "cp taint mem2reg: tainting register %d addr 0x%08x value 0x%08x base val %d index val %d\n",
		reg, addr, *location, global_regs[base], global_regs[index]);
    }
#endif
    global_regs[reg] = result;
}

void cp_taint_mem2reg_10byte(UINT32 addr, REG reg)
{
    UINT32 pageindex;
    UINT32 offset;
    char* page;
    UINT32* location;
    UINT16* location2;
    int result;
    slowpath++;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = (char*) pages[pageindex];        
    page += offset;
    location = (UINT32*) page;
    result = (*location > 0);
    taint_store.wp.read_fault(addr, 10);

    addr += 4;
    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = (char*) pages[pageindex];        
    page += offset;
    location = (UINT32*) page;
    result |= (*location > 0);


    addr += 4;
    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = (char*) pages[pageindex];        
    page += offset;
    location2 = (UINT16*) page;
    result |= (*location2 > 0);

#if TAINT_DEBUG
    if(result > 0) {
	fprintf(logfd, "cp taint mem2reg: tainting register %d addr 0x%08x value 0x%08x base val %d index val %d\n",
		reg, addr, *location, global_regs[base], global_regs[index]);
    }
#endif
    global_regs[reg] = result;
}



void cp_taint_mem2reg_16byte(UINT32 addr, REG reg)
{
    UINT32 pageindex;
    UINT32 offset;
    char* page;
    UINT32* location;
    int result;
    slowpath++;

    pageindex = (addr >> PAGE_SHIFT);
    page = (char*) pages[pageindex];        
    offset = addr & PAGE_MASK;
    page += offset;
    location = (UINT32*) page;
    result = (*location > 0);
    taint_store.wp.read_fault(addr, 16);

    addr += 4;
    pageindex = (addr >> PAGE_SHIFT);
    page = (char*) pages[pageindex];        
    offset = addr & PAGE_MASK;
    page += offset;
    location = (UINT32*) page;
    result |= (*location > 0);

    addr += 4;
    pageindex = (addr >> PAGE_SHIFT);
    page = (char*) pages[pageindex];        
    offset = addr & PAGE_MASK;
    page += offset;
    location = (UINT32*) page;
    result |= (*location > 0);

    addr += 4;
    pageindex = (addr >> PAGE_SHIFT);
    page = (char*) pages[pageindex];        
    offset = addr & PAGE_MASK;
    page += offset;
    location = (UINT32*) page;
    result |= (*location > 0);

#if TAINT_DEBUG
    if(result > 0) {
	fprintf(logfd, "cp taint mem2reg: tainting register %d addr 0x%08x value 0x%08x base val %d index val %d\n",
		reg, addr, *location, global_regs[base], global_regs[index]);
    }
#endif
    global_regs[reg] = result;
}

void cp_taint_reg2mem_1byte(UINT32 addr, REG reg)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];        

    slowpath++;
    if((global_regs[reg] == 0) &&
       (pages[pageindex] == ZERO_PAGE_LOC)) {
	return;
    }

    if(global_regs[reg])
        taint_store.wp.add_read_wp(addr, 1);

    page = get_real_page(pageindex);
    page[offset] = (global_regs[reg]);
}

void cp_taint_reg2mem_2byte(UINT32 addr, REG reg)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];        
    UINT16* location;

    slowpath++;
    if((global_regs[reg] == 0) &&
       (pages[pageindex] == ZERO_PAGE_LOC)) {
	return;
    }

    if(global_regs[reg])
        taint_store.wp.add_read_wp(addr, 2);

    page = get_real_page(pageindex);
    page += offset;
    location = (UINT16*) page;

    if(global_regs[reg]) {
	*location = TWO_BYTE_TAINTED;
    } else {
	*location = 0;
    }
}

void cp_taint_reg2mem_4byte(UINT32 addr, REG reg)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];        
    UINT32* location;

    slowpath++;
    if((global_regs[reg] == 0) &&
       (pages[pageindex] == ZERO_PAGE_LOC)) {
	return;
    }

    if(global_regs[reg])
        taint_store.wp.add_read_wp(addr, 4);

    page = get_real_page(pageindex);
    page += offset;
    location = (UINT32*) page;
    
    if(global_regs[reg]) {
	DPRINT(logfd, "reg tainted cp taint to addr 0x%08x reg %d val %d\n", 
	       addr, reg, global_regs[reg]);
	*location = FOUR_BYTE_TAINTED;
    } else {
	*location = 0;
    }
}

void cp_taint_reg2mem_8byte(UINT32 addr, REG reg)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset;
    char* page = (char*) pages[pageindex];        
    UINT32* location;
    UINT32* location2;

    slowpath++;
    if((global_regs[reg] == 0) &&
       (pages[pageindex] == ZERO_PAGE_LOC)) {
	return;
    }

    if(global_regs[reg])
        taint_store.wp.add_read_wp(addr, 8);

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = get_real_page(pageindex);
    page += offset;
    location = (UINT32*) page;

    addr +=4;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = get_real_page(pageindex);
    page += offset;
    location2 = (UINT32*) page;

    if(global_regs[reg]) {
	DPRINT(logfd, "reg tainted cp taint to addr 0x%08x reg %d val %d\n", 
	       addr, reg, global_regs[reg]);
	*location = FOUR_BYTE_TAINTED;
	*location2 = FOUR_BYTE_TAINTED;
    } else {
	*location = 0;
	*location2 = 0;
    }
}


void cp_taint_reg2mem_10byte(UINT32 addr, REG reg)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset;
    char* page = (char*) pages[pageindex];        
    UINT32* location;
    UINT32* location2;
    UINT16* location3;

    slowpath++;
    if((global_regs[reg] == 0) &&
       (pages[pageindex] == ZERO_PAGE_LOC)) {
	return;
    }

    if(global_regs[reg])
        taint_store.wp.add_read_wp(addr, 10);

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = get_real_page(pageindex);
    page += offset;
    location = (UINT32*) page;

    addr +=4;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = get_real_page(pageindex);
    page += offset;
    location2 = (UINT32*) page;
    
    addr += 4;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = get_real_page(pageindex);
    page += offset;
    location3 = (UINT16*) page;

    if(global_regs[reg]) {
	DPRINT(logfd, "reg tainted cp taint to addr 0x%08x reg %d val %d\n", 
	       addr, reg, global_regs[reg]);
	*location = FOUR_BYTE_TAINTED;
	*location2 = FOUR_BYTE_TAINTED;
	*location3 = TWO_BYTE_TAINTED;
    } else {
	*location = 0;
	*location2 = 0;
	*location3 = 0;
    }
}



void cp_taint_reg2mem_16byte(UINT32 addr, REG reg)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset;
    char* page = (char*) pages[pageindex];        
    UINT32* location;
    UINT32* location2;
    UINT32* location3;
    UINT32* location4;

    slowpath++;
    if((global_regs[reg] == 0) &&
       (pages[pageindex] == ZERO_PAGE_LOC)) {
	return;
    }

    if(global_regs[reg])
        taint_store.wp.add_read_wp(addr, 16);

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = get_real_page(pageindex);
    page += offset;
    location = (UINT32*) page;

    addr +=4;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = get_real_page(pageindex);
    page += offset;
    location2 = (UINT32*) page;

    addr +=4;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = get_real_page(pageindex);
    page += offset;
    location3 = (UINT32*) page;


    addr +=4;

    pageindex = (addr >> PAGE_SHIFT);
    offset = addr & PAGE_MASK;
    page = get_real_page(pageindex);
    page += offset;
    location4 = (UINT32*) page;


    if(global_regs[reg]) {
	DPRINT(logfd, "reg tainted cp taint to addr 0x%08x reg %d val %d\n", 
	       addr, reg, global_regs[reg]);
	*location = FOUR_BYTE_TAINTED;
	*location2 = FOUR_BYTE_TAINTED;
	*location3 = FOUR_BYTE_TAINTED;
	*location4 = FOUR_BYTE_TAINTED;
    } else {
	*location = 0;
	*location2 = 0;
	*location3 = 0;
	*location4 = 0;
    }
}

void cp_taint_reg2mem_generic(u_int reg, u_int addr, u_int numbytes)
{
    u_int pageindex = (addr >> PAGE_SHIFT);
    u_int loc = addr & PAGE_MASK;
    u_int space = PAGE_SIZE - loc;
    char* page;
    u_int len;
    int regval;
    
    slowpath++;
    reallyslow++;
    regval = global_regs[reg];
    if(regval)
        taint_store.wp.add_read_wp(addr, numbytes);

    while(numbytes) {
	loc = addr & PAGE_MASK;
	space = PAGE_SIZE - loc;
	DPRINT(stderr, "loc within page is %u space is %u written is %u\n", loc, space, numbytes);
	if(numbytes > space) {
	    len = space;
	} else {
	    len = numbytes;
	}
	if(regval == 0 && pages[pageindex] == ZERO_PAGE) {
	    numbytes -= len;
	    addr += len;
	    pageindex++;
	    continue;
	}
	page = get_real_page(pageindex);
	//taint the bytes read in for this page
	page += loc;
	memset(page, regval, len);
	DPRINT(logfd, "tainting addr 0x%x len %u\n", (u_int)page, len);
	numbytes = numbytes - len;
	addr += len;
	pageindex++;
    }
}

void cp_taint_mem2mem_1byte(UINT32 srcaddr, UINT32 destaddr)
{
    UINT32 pi1 = (srcaddr >> PAGE_SHIFT);
    UINT32 pi2 = (destaddr >> PAGE_SHIFT);
    UINT32 offset1 = srcaddr & PAGE_MASK;
    UINT32 offset2 = destaddr & PAGE_MASK;
    char* srcpge = (char*) pages[pi1];   
    char* destpge = (char*) pages[pi2];   
    INT32 result;

    slowpath++;
    destpge = get_real_page(pi2);
    result = (srcpge[offset1]);// | destpge[offset2]);
    taint_store.wp.read_fault(srcaddr, 1);
    if(result) {
	destpge[offset2] = ONE_BYTE_TAINTED;
    taint_store.wp.add_read_wp(destaddr, 1);
    } else {
	destpge[offset2] = 0;
    taint_store.wp.rm_watch(destaddr, 1);
    }
}

void cp_taint_mem2mem_2byte(UINT32 srcaddr, UINT32 destaddr)
{
    UINT32 pi1 = (srcaddr >> PAGE_SHIFT);
    UINT32 pi2 = (destaddr >> PAGE_SHIFT);
    UINT32 offset1 = srcaddr & PAGE_MASK;
    UINT32 offset2 = destaddr & PAGE_MASK;
    char* page1 = (char*) pages[pi1];   
    char* page2 = (char*) pages[pi2];   
    UINT16* srctaint;
    UINT16* desttaint;

    slowpath++;
    page2 = get_real_page(pi2);

    page1 += offset1;
    srctaint = (UINT16*) page1;
    taint_store.wp.read_fault(srcaddr, 2);

    page2 += offset2;
    desttaint = (UINT16*) page2;
    if(*srctaint)
        taint_store.wp.add_read_wp(destaddr, 2);
    else
        taint_store.wp.rm_watch(destaddr, 2);

    *desttaint = *srctaint;
}

void cp_taint_mem2mem_4byte(UINT32 srcaddr, UINT32 destaddr)

{
    UINT32 pi1 = (srcaddr >> PAGE_SHIFT);
    UINT32 pi2 = (destaddr >> PAGE_SHIFT);
    UINT32 offset1 = srcaddr & PAGE_MASK;
    UINT32 offset2 = destaddr & PAGE_MASK;
    char* page1 = (char*) pages[pi1];   
    char* page2 = (char*) pages[pi2];   
    UINT32* srctaint;
    UINT32* desttaint;

    slowpath++;
    page2 = get_real_page(pi2);

    page1 += offset1;
    srctaint = (UINT32*) page1;
    taint_store.wp.read_fault(srcaddr, 4);

    page2 += offset2;
    desttaint = (UINT32*) page2;
    if(*srctaint)
        taint_store.wp.add_read_wp(destaddr, 4);
    else
        taint_store.wp.rm_watch(destaddr, 4);

    *desttaint = *srctaint;
}

void cp_taint_mem2mem_generic(u_int srcaddr, u_int destaddr, u_int numbytes)
{
    u_int pi1 = (srcaddr >> PAGE_SHIFT);
    u_int pi2 = (destaddr >> PAGE_SHIFT);
    char* page1 = (char*) pages[pi1];   
    char* page2 = (char*) pages[pi2];   
    u_int toread = numbytes;
    u_int towrite = numbytes;
    u_int loc;
    u_int space;
    u_int len;
    char taint[numbytes];
    char* p;

    loc = srcaddr & PAGE_MASK;
    space = PAGE_SIZE - loc;
    if((pages[pi1] == ZERO_PAGE_LOC) && (space >= numbytes)) {
	loc = destaddr & PAGE_MASK;
	space = PAGE_SIZE - loc;
	if((pages[pi2] == ZERO_PAGE_LOC) && (space >=numbytes)) {
	    return;
	}
    }
    reallyslow++;
    //first read the taint in from the src addr
    //then write the taint out
    p = taint;
    while(toread) {
	page1 = get_real_page(pi1);

	loc = srcaddr & PAGE_MASK;
	space = PAGE_SIZE - loc;

	if(toread > space) {
	    len = space;
	} else {
	    len = toread;
	}
    taint_store.wp.read_fault(loc, len);
	page1 += loc;
	memcpy(p, page1, len);
	p += len;
	toread = toread - len;
	pi1++;
	srcaddr += len;
    }
    p = taint;
    while(towrite) {
	page2 = get_real_page(pi2);

	loc = destaddr & PAGE_MASK;
	space = PAGE_SIZE - loc;

	if(towrite > space) {
	    len = space;
	} else {
	    len = towrite;
	}
	page2 += loc;
    for(unsigned int i = 0; i < len; i++) {
        if (*(p+i))
            taint_store.wp.add_read_wp((ADDRINT)(page2+i), 1);
        else
            taint_store.wp.rm_read((ADDRINT)(page2+i), 1);
    }
	memcpy(page2, p, len);
	p += len;
	towrite = towrite - len;
	destaddr += len;
	pi2++;
    }
}

static inline BOOL addr_tainted_1byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];
    taint_store.wp.read_fault(addr, 1);

    return (page[offset]  > 0);
}

static inline BOOL addr_tainted_2byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];   
    UINT16* tainted;
    taint_store.wp.read_fault(addr, 2);

    page += offset;
    tainted = (UINT16*) page;

    return (*tainted > 0); 
}

static inline BOOL addr_tainted_4byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];   
    UINT32* tainted;
    taint_store.wp.read_fault(addr, 4);
    
    page += offset;
    tainted = (UINT32*) page;

    DPRINT(logfd, "is_addr_tainted_4byte returns %d value is 0x%08x addr 0x%08x offset %u pageindex %u\n"
	    "page addr is 0x%08x ZERO PAGE is 0x%08x\n", 
	    (*tainted > 0), *tainted, addr, offset, pageindex, (u_int) pages[pageindex], (u_int) ZERO_PAGE_LOC);
    return (*tainted > 0);
}

static inline BOOL addr_tainted_8byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];   
    UINT32* tainted;
    UINT32* tainted2;
    taint_store.wp.read_fault(addr, 8);
    
    page += offset;
    tainted = (UINT32*) page;

    page += 4;
    tainted2 = (UINT32*) page;

    *tainted = *tainted | *tainted2;

    DPRINT(logfd, "is_addr_tainted_4byte returns %d value is 0x%08x addr 0x%08x offset %u pageindex %u\n"
	    "page addr is 0x%08x ZERO PAGE is 0x%08x\n", 
	    (*tainted > 0), *tainted, addr, offset, pageindex, (u_int) pages[pageindex], (u_int) ZERO_PAGE_LOC);
    return (*tainted > 0);
}

static inline BOOL addr_tainted_10byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];   
    UINT32* tainted;
    UINT32* tainted2;
    UINT16* tainted3;
    taint_store.wp.read_fault(addr, 10);
    
    page += offset;
    tainted = (UINT32*) page;

    page += 4;
    tainted2 = (UINT32*) page;

    page += 4;
    tainted3 = (UINT16*) page;


    *tainted = *tainted | *tainted2 | *tainted3;

    DPRINT(logfd, "is_addr_tainted_4byte returns %d value is 0x%08x addr 0x%08x offset %u pageindex %u\n"
	    "page addr is 0x%08x ZERO PAGE is 0x%08x\n", 
	    (*tainted > 0), *tainted, addr, offset, pageindex, (u_int) pages[pageindex], (u_int) ZERO_PAGE_LOC);
    return (*tainted > 0);
}


static inline BOOL addr_tainted_16byte(UINT32 addr)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];   
    UINT32* tainted;
    UINT32* tainted2;
    taint_store.wp.read_fault(addr, 16);
    
    page += offset;
    tainted = (UINT32*) page;

    page += 4;
    tainted2 = (UINT32*) page;
    *tainted = *tainted | *tainted2;

    page += 4;
    tainted2 = (UINT32*) page;
    *tainted = *tainted | *tainted2;

    page += 4;
    tainted2 = (UINT32*) page;
    *tainted = *tainted | *tainted2;

    DPRINT(logfd, "is_addr_tainted_4byte returns %d value is 0x%08x addr 0x%08x offset %u pageindex %u\n"
	    "page addr is 0x%08x ZERO PAGE is 0x%08x\n", 
	    (*tainted > 0), *tainted, addr, offset, pageindex, (u_int) pages[pageindex], (u_int) ZERO_PAGE_LOC);
    return (*tainted > 0);
}

BOOL is_addr_tainted_generic(UINT32 addr,  UINT32 numbytes)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    char* page = (char*) pages[pageindex];
    UINT32 byte = (addr & PAGE_MASK);
    UINT32 space = PAGE_SIZE - byte;
    char* p;
    UINT32 i;
    taint_store.wp.read_fault(addr, numbytes);

    fprintf(logfd, "executing generic fcn!\n");
    if(numbytes > space) {
	fprintf(errfd, "ACK! is addr tainted crossing page boundaries!!\n");
	fprintf(stderr, "is addr tainted crossing page boundaries!!\n");
	exit(0);
    }

    if(page == ZERO_PAGE_LOC) return 0;

    page = (char*) pages[pageindex];
    p = page + byte;
    DPRINT(logfd, "checking addr 0x%x len %u\n", (u_int) p, numbytes);
    for(i = byte; i < byte+numbytes; i++) {
	if(page[i]) {
	    return 1;
	}
    }
    return 0;
}

BOOL is_addr_tainted_1byte(UINT32 addr)
{
    return addr_tainted_1byte(addr);
}

BOOL is_addr_tainted_2byte(UINT32 addr)
{
    return addr_tainted_2byte(addr);
}

BOOL is_addr_tainted_4byte(UINT32 addr)
{
    return addr_tainted_4byte(addr);
}

BOOL is_addr_tainted_8byte(UINT32 addr)
{
    return addr_tainted_8byte(addr);
}

BOOL is_addr_tainted_16byte(UINT32 addr)
{
    return addr_tainted_16byte(addr);
}

BOOL do_reg_addr_differ_1byte(UINT32 addr, REG reg)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_1byte(addr);
    r1 = r1 ^ r2;

    DPRINT(logfd, "do_reg_addr_differ_1byte returns %d\n", r1);
    return (r1);    
}

BOOL do_reg_addr_differ_2byte(UINT32 addr, REG reg)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    DPRINT(logfd, "do_reg_addr_read_differ_2byte returns %d\n", r1);
    return (r1); 
}


BOOL do_reg_addr_differ_4byte(UINT32 addr, REG reg)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    DPRINT(logfd, "do_reg_addr_read_differ_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_differ_10byte(UINT32 addr, REG reg)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_10byte(addr);
    r1 = r1 ^ r2;

    DPRINT(logfd, "do_reg_addr_read_differ_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_differ_8byte(UINT32 addr, REG reg)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_8byte(addr);
    r1 = r1 ^ r2;

    DPRINT(logfd, "do_reg_addr_read_differ_4byte returns %d\n", r1);
    return (r1);
}


BOOL do_reg_addr_differ_16byte(UINT32 addr, REG reg)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_16byte(addr);
    r1 = r1 ^ r2;

    DPRINT(logfd, "do_reg_addr_read_differ_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_differ_generic(UINT32 addr, UINT32 size, REG reg)
{
    fprintf(logfd, "executing generic fcn!\n");
    return 0;
}

BOOL do_addr_differ_1byte(UINT32 addr1, UINT32 addr2)
{
    int r1, r2;

    r1 = addr_tainted_1byte(addr1);
    r2 = addr_tainted_1byte(addr2);
    r1 = r1 ^ r2;

    DPRINT(logfd, "do_addr_1byte returns %d\n", r1);
    return (r1);
}

BOOL do_addr_differ_2byte(UINT32 addr1, UINT32 addr2)
{
    int r1, r2;

    r1 = addr_tainted_2byte(addr1);
    r2 = addr_tainted_2byte(addr2);
    r1 = r1 ^ r2;

    DPRINT(logfd, "do_addr_differ_2byte returns %d\n", r1);
    return (r1);

}

BOOL do_addr_differ_4byte(UINT32 addr1, UINT32 addr2)
{
    int r1, r2;

    r1 = addr_tainted_4byte(addr1);
    r2 = addr_tainted_4byte(addr2);
    r1 = r1 ^ r2;

    DPRINT(logfd, "do_addr_differ_4byte returns %d\n", r1);
    return (r1);

}

void taint_reg(REG reg)
{
#ifdef TAINT_DEBUG
    if(reg > REGS_AVAIL) {
	fprintf(logfd, "reg %d greater than avail reg!!!\n", reg);
	exit(0);
    }
#endif
    DPRINT(logfd, "taint_reg tainting %d\n", reg);

    slowpath++;
    global_regs[reg] = 1;
}

void clear_reg(REG reg)
{
#ifdef TAINT_DEBUG
    if(reg > REGS_AVAIL) {
	fprintf(logfd, "reg %d greater than avail reg!!!\n", reg);
	exit(0);
    }
#endif
    global_regs[reg] = 0;
}

BOOL is_reg_tainted(REG reg)
{
#ifdef TAINT_DEBUG
    if(reg > REGS_AVAIL) {
	fprintf(logfd, "reg %d greater than avail reg!!!\n", reg);
	exit(0);
    }
#endif
    return global_regs[reg];
}

BOOL is_reg_clear(REG reg)
{
    return !global_regs[reg];
}

void match_regs(REG dstreg, REG srcreg)
{
    global_regs[dstreg] = global_regs[srcreg];
}

void xchg_regs(REG reg1, REG reg2)
{
    int tmp;

    tmp = global_regs[reg1];
    global_regs[reg1] = global_regs[reg2];
    global_regs[reg2] = tmp;
}

void xadd_regs(REG reg1, REG reg2)
{
    int tmp;

    tmp = global_regs[reg1] | global_regs[reg2];
    global_regs[reg2] = global_regs[reg1];
    global_regs[reg1] = tmp;
}

void xadd_reg_mem_1byte(REG reg, UINT32 addr)
{
    int tmp;
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];
    int r1;

    slowpath++;
    
    page = get_real_page(pageindex);
    r1 = (page[offset] > 0);
    tmp = global_regs[reg];

    global_regs[reg] = r1;    
#if TAINT_DEBUG
    if(global_regs[reg] > 0) {
	fprintf(logfd, "xadd: taiting reg %d\n", reg);
    }
#endif
    if(tmp) {
	page[offset] = ONE_BYTE_TAINTED;
    }
}

void xadd_reg_mem_2byte(REG reg, UINT32 addr)
{
    int tmp;
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];
    UINT16* location;
    int r1;

    slowpath++;

    page = get_real_page(pageindex);
    page += offset;
    location = (UINT16*) page;
    r1 = (*location > 0);
    tmp = global_regs[reg];

    global_regs[reg] = r1;    
#if TAINT_DEBUG
    if(global_regs[reg] > 0) {
	fprintf(logfd, "xadd: taiting reg %d\n", reg);
    }
#endif
    if(tmp) {
	*location = TWO_BYTE_TAINTED;
    }
}

void xadd_reg_mem_4byte(REG reg, UINT32 addr)
{
    int tmp;
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];   
    UINT32* location;
    int r1;

    slowpath++;

    page = get_real_page(pageindex);
    page += offset;
    location = (UINT32*) page;
    r1 = (*location > 0);
    tmp = global_regs[reg];

    global_regs[reg] = r1;    
#if TAINT_DEBUG
    if(global_regs[reg] > 0) {
	fprintf(logfd, "xadd: taiting reg %d\n", reg);
    }
#endif
    if(tmp) {
	*location = FOUR_BYTE_TAINTED;
    }

}

void xadd_reg_mem_generic(REG reg, UINT32 addr, UINT32 size)
{
    fprintf(errfd, "xadd with greater than 32bit values %u bytes!!!\n", size);
}

/*
  the registers used to calculate the EA will taint both the memory loc
  and the regsiter exchanging values
*/
void xchg_reg_mem_1byte(REG reg, UINT32 addr)
{
    int tmp;
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];
    slowpath++;

    page = get_real_page(pageindex);
    tmp = global_regs[reg];
    if(page[offset]) {
	global_regs[reg] = 1;
    } else {
	global_regs[reg] = 0;
    }
    if(tmp) {
	page[offset] = ONE_BYTE_TAINTED;
    } else {
	page[offset] = 0;
    }
} 

void xchg_reg_mem_2byte(REG reg, UINT32 addr)
{
   int tmp;
   UINT32 pageindex = (addr >> PAGE_SHIFT);
   UINT32 offset = addr & PAGE_MASK;
   char* page = (char*) pages[pageindex];
   UINT16* location;

   slowpath++;

   page = get_real_page(pageindex);
   page += offset;
   location = (UINT16*) page;
   tmp = global_regs[reg];
   if(*location) {
       global_regs[reg] = 1;
   } else {
       global_regs[reg] = 0;
   }

   if(tmp) {
       *location = TWO_BYTE_TAINTED;
   } else {
       *location = 0;
   }
}

void xchg_reg_mem_4byte(REG reg, UINT32 addr)
{
   int tmp;
   UINT32 pageindex = (addr >> PAGE_SHIFT);
   UINT32 offset = addr & PAGE_MASK;
   char* page = (char*) pages[pageindex];   
   UINT32* location;

   slowpath++;

   page = get_real_page(pageindex);
   page += offset;
   location = (UINT32*) page;
   tmp = global_regs[reg];
   if(*location) {
       DPRINT(logfd, "xchg_reg_mem_4byte tainting reg %d addr val 0x%08x addr 0x%08x\n", 
	       reg, *location, addr);
       global_regs[reg] = 1;
   } else {
       global_regs[reg] = 0;
   }

   if(tmp) {
       *location = FOUR_BYTE_TAINTED;
   } else {
       *location = 0;
   }
} 

void xchg_reg_mem_generic(REG reg, UINT32 addr, UINT32 size)
{
    fprintf(errfd, "xchg called with vars larger than 32 bits!!!\n");
    
} 
BOOL are_regs_tainted(REG reg1, UINT32 reg1stat, REG reg2, UINT32 reg2stat, 
		      REG reg3, UINT32 reg3stat)
{
    int tainted;
    int r1, r2, r3;

    r1 = (reg1stat & REG_READ);
    r1 = (r1 == REG_READ);
    r1 = r1 & global_regs[reg1];

    r2 = (reg2stat & REG_READ);
    r2 = (r2 == REG_READ);
    r2 = r2 & global_regs[reg2];

    r3 = (reg3stat & REG_READ);
    r3 = (r3 == REG_READ);
    r3 = r3 & global_regs[reg3];
    tainted  =  r1 | r2 | r3;

    DPRINT(logfd, "are_regs-tainted  returns %d\n", tainted);
    return tainted;
}

void taint_dest_regs(REG reg1, UINT32 reg1stat, REG reg2, UINT32 reg2stat, 
		     REG reg3, UINT32 reg3stat)
{
    int written;
    slowpath++;
    written = (reg1stat & REG_WRITE);
    written = (written == REG_WRITE);
    global_regs[reg1] = written;

    written = (reg2stat & REG_WRITE);
    written = (written == REG_WRITE);
    global_regs[reg2] = written;

    written = (reg3stat & REG_WRITE);
    written = (written == REG_WRITE);
    global_regs[reg3] = written;
    DPRINT(logfd, "taiting dest regs!\n");
}

/*is either registers OR memory tained? */
BOOL are_regs_mem_read_tainted_1byte(UINT32 addr, REG reg1, UINT32 reg1stat, 
				     REG reg2, UINT32 reg2stat, REG reg3, 
				     UINT32 reg3stat)
{
    int tainted;
    int r1, r2, r3;
 
/*
    what we are trying to do without conditional code
    tainted  =  ((reg1stat & REG_READ) && is_reg_tainted(reg1));
    tainted |= ((reg2stat & REG_READ) && is_reg_tainted(reg2));
    tainted |= ((reg3stat & REG_READ) && is_reg_tainted(reg3)); 
    return (taintsrc || (pages[pageindex] && page[offset]))
*/
  
    r1 = (reg1stat & REG_READ);
    r1 = (r1 == REG_READ);
    r1 = r1 & global_regs[reg1];

    r2 = (reg2stat & REG_READ);
    r2 = (r2 == REG_READ);
    r2 = r2 & global_regs[reg2];

    r3 = (reg3stat & REG_READ);
    r3 = (r3 == REG_READ);
    r3 = r3 & global_regs[reg3];
    tainted  =  r1 | r2 | r3;
    
    r1 = addr_tainted_1byte(addr);
    tainted |= r1;

    DPRINT(logfd, "are_regs_mem_tainted_1byte returns %d\n", tainted);
    return(tainted);
}

BOOL are_regs_mem_read_tainted_2byte(UINT32 addr, REG reg1, UINT32 reg1stat, 
				     REG reg2, UINT32 reg2stat, REG reg3, 
				     UINT32 reg3stat)
{
    int tainted;
    int r1, r2, r3;

    r1 = (reg1stat & REG_READ);
    r1 = (r1 == REG_READ);
    r1 = r1 & global_regs[reg1];

    r2 = (reg2stat & REG_READ);
    r2 = (r2 == REG_READ);
    r2 = r2 & global_regs[reg2];

    r3 = (reg3stat & REG_READ);
    r3 = (r3 == REG_READ);
    r3 = r3 & global_regs[reg3];
    tainted  =  r1 | r2 | r3;
    
    r1 = addr_tainted_2byte(addr);
    tainted |= r1;

    DPRINT(logfd, "are_regs_mem_tainted_2byte returns %d\n", tainted);
    return (tainted);
}
BOOL are_regs_mem_read_tainted_4byte(UINT32 addr, REG reg1, UINT32 reg1stat, 
				     REG reg2, UINT32 reg2stat, REG reg3, 
				     UINT32 reg3stat)
{
    int tainted;
    int r1, r2, r3;

    r1 = (reg1stat & REG_READ);
    r1 = (r1 == REG_READ);
    r1 = r1 & global_regs[reg1];

    r2 = (reg2stat & REG_READ);
    r2 = (r2 == REG_READ);
    r2 = r2 & global_regs[reg2];

    r3 = (reg3stat & REG_READ);
    r3 = (r3 == REG_READ);
    r3 = r3 & global_regs[reg3];
    tainted  =  r1 | r2 | r3;
    
    r1 = addr_tainted_4byte(addr);
    tainted |= r1;

    DPRINT(logfd, "are_regs_mem_tainted_4byte returns %d\n", tainted);
    return (tainted);
}

BOOL are_regs_mem_read_tainted_8byte(UINT32 addr, REG reg1, UINT32 reg1stat, 
				     REG reg2, UINT32 reg2stat, REG reg3, 
				     UINT32 reg3stat)
{
    int tainted;
    int r1, r2, r3;

    r1 = (reg1stat & REG_READ);
    r1 = (r1 == REG_READ);
    r1 = r1 & global_regs[reg1];

    r2 = (reg2stat & REG_READ);
    r2 = (r2 == REG_READ);
    r2 = r2 & global_regs[reg2];

    r3 = (reg3stat & REG_READ);
    r3 = (r3 == REG_READ);
    r3 = r3 & global_regs[reg3];
    tainted  =  r1 | r2 | r3;
    
    r1 = addr_tainted_8byte(addr);
    tainted |= r1;

    DPRINT(logfd, "are_regs_mem_tainted_4byte returns %d\n", tainted);
    return (tainted);
}

BOOL are_regs_mem_read_tainted_16byte(UINT32 addr, REG reg1, UINT32 reg1stat, 
				     REG reg2, UINT32 reg2stat, REG reg3, 
				     UINT32 reg3stat)
{
    int tainted;
    int r1, r2, r3;

    r1 = (reg1stat & REG_READ);
    r1 = (r1 == REG_READ);
    r1 = r1 & global_regs[reg1];

    r2 = (reg2stat & REG_READ);
    r2 = (r2 == REG_READ);
    r2 = r2 & global_regs[reg2];

    r3 = (reg3stat & REG_READ);
    r3 = (r3 == REG_READ);
    r3 = r3 & global_regs[reg3];
    tainted  =  r1 | r2 | r3;
    
    r1 = addr_tainted_16byte(addr);
    tainted |= r1;

    DPRINT(logfd, "are_regs_mem_tainted_4byte returns %d\n", tainted);
    return (tainted);
}

BOOL are_regs_mem_tainted_generic(UINT32 addr, UINT32 size, REG reg1, UINT32 reg1stat, 
				  REG reg2, UINT32 reg2stat, REG reg3, UINT32 reg3stat)
{
    fprintf(logfd, "executing generic fcn!\n");
    return(are_regs_tainted(reg1, reg1stat, reg2, reg2stat, reg3, reg3stat) ||
	   is_addr_tainted_generic(addr, size));
}

//static INT32  read_fd;
static INT32  socket_type;
static UINT32 rd_buffer_addr;
static UINT32 sckt_buffer_addr;
static INT32  valid_socketcall;
static INT32  valid_readcall;
static INT32 aftermain = 0;

void getsysnum(INT32 syscall_num)
{
    fprintf(logfd, "syscall number %d\n", syscall_num);
}

//need to clear page if read overwrites tatined addresses
//void check_read_syscall(INT32 syscall_num, void * arg0, void * arg1)
void check_read_syscall(ADDRINT syscall_num, ADDRINT arg1)
{
    INT32 r1;
    INT32 r2;
#if 0
    fprintf(logfd, "syscall num %d\n", syscall_num);
    fflush(logfd);
#endif
    r1 = (syscall_num == SYS_read);
    r2 = aftermain;
    r1 = r1 & r2;
    rd_buffer_addr = (UINT32) arg1;
    valid_readcall = r1;
}

BOOL is_read_syscall(void)
{
    return (valid_readcall == 1);
}

void taint_read_result(ADDRINT retval)
{
    INT32 result = (INT32) retval;
    if(result > 0) {
	DPRINT(logfd, "tainting memory from loc 0x%08x num bytes %d\n", 
		rd_buffer_addr, result);
	taint_addr_generic(rd_buffer_addr, result);
    }
}

BOOL is_socketcall_syscall(INT32 syscall_num)
{
    valid_socketcall = 0;
    return (syscall_num == SYS_socketcall);
}

//return true if socketcall is accept OR socketfd is tainted
//void get_socketcall_args(void * arg0, void * arg1)
void get_socketcall_args(ADDRINT arg0, ADDRINT arg1)
{
    INT32 r2;

    DPRINT(stderr, "entering socketcall args\n");

    unsigned long* args = (unsigned long*) arg1;
    socket_type = (INT32) arg0; //socketcalltype
    sckt_buffer_addr = args[1]; //in buffer

    r2  = (socket_type == SYS_RECV);
    r2 |= (socket_type == SYS_RECVFROM);
    r2 |= (socket_type == SYS_RECVMSG);

    valid_socketcall = r2;
}

BOOL is_valid_socketcall(void)
{
    return (valid_socketcall == 1);
}

void handle_socketcall_result(ADDRINT retval)
{
    INT32 result = (INT32) retval;

    DPRINT(stderr, "entering handle_socketcall_result\n");
    slowpath++;
    if(valid_socketcall == 1) {
        switch(socket_type) {
            case SYS_RECV:
                DPRINT(stderr, "got recv %d\n", result);
                taint_addr_generic(sckt_buffer_addr, result);
                break;
            case SYS_RECVFROM:
                fprintf(errfd, "Got RECVFROM not handling this!!!!\n");
                break;
            case SYS_RECVMSG:
                DPRINT(stderr, "got recv\n");
                taint_addr_generic(sckt_buffer_addr, result);
                break;
        }
    }
}

void breakdown_ins(INS ins)
{
    string instruction;
    int count;
    int opreg;
    int opmem;
    int numbytes;
    int i;
    REG reg = (REG)0;
    string regname;
    UINT32 addr;

    instruction = INS_Disassemble(ins);
    count = INS_OperandCount(ins);
    fprintf(logfd, "%d operands-%s  \n", count, instruction.c_str());

    for(i = 0; i < count; i++) {
	opreg = 0;
	opmem = 0;
	if(INS_OperandIsReg(ins, i)) {
	    reg = INS_OperandReg(ins, i);
	    regname = REG_StringShort(reg);
	    fprintf(logfd, "operand %d is register %s regnum %d", i, regname.c_str(), reg);
	    opreg = 1;
	} else if (INS_OperandIsMemory(ins, i)) {
	    fprintf(logfd, "operand %d is memory ", i);
	    reg = INS_OperandMemoryBaseReg(ins, i);
	    if(REG_valid(reg)) {
		regname = REG_StringShort(reg);
		fprintf(logfd, " base reg is %s", regname.c_str());
	    }
	    reg = INS_OperandMemoryIndexReg(ins, i);
	    if(REG_valid(reg)) {
		regname = REG_StringShort(reg);
		fprintf(logfd, " index reg is %s ", regname.c_str());
	    } else {
		fprintf(logfd, " index reg invalid value is %d\n", reg);
	    }

	    opmem = 1;
	} else if(INS_OperandIsImmediate(ins, i)){
	    fprintf(logfd, "operand %d is immediate\n", i);
	} else if(INS_IsIpRelRead(ins)) {
	    fprintf(logfd, "operand %d is ip relative read\n", i);
	} else {
	    if(INS_IsDirectBranchOrCall(ins)) {
		ADDRINT insaddr;
		insaddr = INS_Address(ins);
		addr = INS_DirectBranchOrCallTargetAddress(ins);
		fprintf(logfd, "direct branch or call target is 0x%08x ins address located at 0x%08x\n", addr, insaddr);
	    } else {
		fprintf(logfd, "operand %d is unknown\n", i);
	    }
	}
	if(opreg) {
	    if(INS_RegRContain(ins, reg)) {
		fprintf(logfd, " that is read\n");
	    }
	    if(INS_RegWContain(ins, reg)) {
		fprintf(logfd, " that is written\n");
	    } else {
		fprintf(logfd, "\n");
	    }
	} else if(opmem) {
	    if(INS_IsMemoryRead(ins)) {
		numbytes = INS_MemoryReadSize(ins);
		fprintf(logfd, " that is memory read %d bytes\n", numbytes);
	    }
	    if(INS_IsMemoryWrite(ins)) {
		numbytes = INS_MemoryWriteSize(ins);
		fprintf(logfd, " that is memory write %d bytes\n", numbytes);
	    }
	}
    }    
}

void Fini(INT32, void*);

void compromised(UINT32 addr, UINT32 numbytes)
{
    UINT32 pageindex = (addr >> PAGE_SHIFT);
    UINT32 offset = addr & PAGE_MASK;
    char* page = (char*) pages[pageindex];   
    UINT32* tainted;
    
    if(addr > 0) {
	page += offset;
	tainted = (UINT32*) page;
	
	fprintf(logfd, "is_addr_tainted_4byte returns %d value is 0x%08x addr 0x%08x offset %u pageindex %u\n"
		"page addr is 0x%08x ZERO PAGE is 0x%08x numbytes %u\n", 
		(*tainted > 0), *tainted, addr, offset, pageindex, (u_int) pages[pageindex], (u_int) ZERO_PAGE_LOC, numbytes);
    }
    fprintf(logfd, "Taint attack!  We've been compromised!\n");
    //    fprintf(stderr, "Taint attack!  We've been compromised!\n");
    Fini((INT32)0, (void *)0);
    exit(0);
}

//not used
void Check_TaintMemRet(ADDRINT readloc, ADDRINT readsize)
{
//    u_int* returnaddr = (u_int*) readloc;

    //DPRINT(logfd, "stack location is 0x%x value is 0x%x\n", 
	    //readloc, *returnaddr);
}

void instrument_floatload(INS ins)
{
    USIZE numbytes;
    REG reg    = INS_OperandReg(ins, 1);
    REG srcreg;

    if(INS_OperandIsReg(ins, 0)) {
	srcreg = INS_OperandReg(ins, 1);
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(match_regs),
		       IARG_UINT32, reg, IARG_UINT32, srcreg, IARG_END);
    } else {
	numbytes = INS_MemoryReadSize(ins);
	switch(numbytes) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END); 
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_2byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_4byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END); 
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);

	    break;
	case 8:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_8byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END); 
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_8byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);

	    break;
	case 10:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_10byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END); 
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_10byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	default:
	    fprintf(stderr, "floatload instruction greater than 32 bits size is %u\n", numbytes);
	    fflush(stderr);
	}
    }
}    

void instrument_floatstore(INS ins)
{
    USIZE numbytes;
    REG reg  = INS_OperandReg(ins, 0);
    REG dstreg;
    
    if(INS_OperandIsReg(ins, 1)) {
	dstreg = INS_OperandReg(ins, 1);
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(match_regs),
		       IARG_UINT32, dstreg, IARG_UINT32, reg, IARG_END);
    } else {
	numbytes = INS_MemoryWriteSize(ins);
	switch(numbytes) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_2byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_2byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_4byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END); 
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_4byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);

	    break;
	case 8:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_8byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END); 
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_8byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);

	    break;
	case 10:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_10byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END); 
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_10byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);

	    break;
	default:
	    fprintf(stderr, "float store instruction greater than 32 bits size is %u\n", numbytes);
	    fflush(stderr);
	}
    }
}

void instrument_fldconstant(INS ins)
{
    REG dstreg  = INS_OperandReg(ins, 0);
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(clear_reg),
		   IARG_UINT32, dstreg, IARG_END);
}

void instrument_pcmp(INS ins)
{
    REG dstreg  = INS_OperandReg(ins, 0);

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(clear_reg),
		   IARG_UINT32, dstreg, IARG_END);
}

void instrument_mov(INS ins)
{
    int ismemread = INS_IsMemoryRead(ins);
    int ismemwrite = INS_IsMemoryWrite(ins);
    USIZE addrsize = 0;
    int immval = 0;
    REG dstreg = REG_INVALID();
    REG reg    = REG_INVALID();

#if 0
    string instruction;
    instruction = INS_Disassemble(ins);
    fprintf(stderr, "move instruction is %s\n", instruction.c_str());
    breakdown_ins(ins);
#endif

    if(INS_IsMemoryRead(ins)) {
	ismemread = 1;
	addrsize = INS_MemoryReadSize(ins);
	reg = INS_OperandReg(ins, 0);
	if(!is_valid_reg(reg)) return;
    } else if(INS_IsMemoryWrite(ins)) {
	ismemwrite = 1;
	addrsize = INS_MemoryWriteSize(ins);
	if(INS_OperandIsReg(ins, 1)) {
	    reg = INS_OperandReg(ins, 1);
	    if(!is_valid_reg(reg)) return;
	} else {
	    if(!INS_OperandIsImmediate(ins, 1)) return;
	    //must be an immediate value
	    immval = 1;
	}
    } else {
	if(!(INS_OperandIsReg(ins, 0))) return;
	dstreg = INS_OperandReg(ins, 0);
	if(!is_valid_reg(dstreg)) return;

	if(INS_OperandIsReg(ins, 1)) {
	    reg = INS_OperandReg(ins, 1);
	    if(!is_valid_reg(reg)) return;
	} else {
 	    //sometimes get an offset into video memory
	    if(!INS_OperandIsImmediate(ins, 1)) return;
	    //must be an immediate value
	    immval = 1;
	}
    }

    //2 (src) operand is memory...destination must be a register
    if(ismemread) {
	DPRINT(logfd, "instrument mov is mem read: reg is num %d size of mem read is %u\n", 
		reg, addrsize);
	switch(addrsize) {
	case 1:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_1byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_1byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_2byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_4byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END); 
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_4byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);

	    break;
	case 8:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_8byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END); 
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_8byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);

	    break;
	case 16:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_16byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END); 
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_16byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);

	    break;
	default:
	    fprintf(logfd, "mov instruction greater than 32 bits size is %u\n", addrsize);
	}
    } else if(ismemwrite) {
	DPRINT(logfd, "instrument mov is mem write: reg is num %d size of mem write is %u\n", 
		reg, addrsize);
	if(immval) {
	    //moving an immediate into memory
	    switch(addrsize) {
	    case 1:
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(clear_addr_1byte),
			       IARG_MEMORYWRITE_EA, IARG_END);
		break;
	    case 2:
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(clear_addr_2byte),
			       IARG_MEMORYWRITE_EA, IARG_END);
		
		break;
 	    case 4:
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(clear_addr_4byte),
			       IARG_MEMORYWRITE_EA, IARG_END);

		break;
 	    case 8:
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(clear_addr_8byte),
			       IARG_MEMORYWRITE_EA, IARG_END);
		break;
 	    case 16:
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(clear_addr_16byte),
			       IARG_MEMORYWRITE_EA, IARG_END);
		break;
	    default:
		fprintf(errfd, "instrument mov: unknown addr write size is %u\n", addrsize);
	    }
	} else {
	    //moving a reg val into memory
	    switch(addrsize) { 
	    case 1:
		INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_1byte),
				 IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
		INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_1byte),
				   IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);

		break;
	    case 2:
		INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_2byte),
				 IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
		INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_2byte),
				   IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
		break;
	    case 4:
		INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_4byte),
				 IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
		INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
				   IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
		break;
	    case 8:
		INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_8byte),
				 IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
		INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_8byte),
				   IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
		break;
	    case 16:
		INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_16byte),
				 IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
		INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_16byte),
				   IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
		break;
	    default:
		fprintf(errfd, "instrument mov: unknown addr write size is %u\n", addrsize);
	    }
	}
    } else {
	if(immval) {
	    //mov immediate value into register
	    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(clear_reg),
			   IARG_UINT32, dstreg, IARG_END);
	} else {
	    //mov one reg val into another
	    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(match_regs),
			   IARG_UINT32, dstreg, IARG_UINT32, reg, IARG_END);
	}
    }
    
}
/*we have a seperate set of reg_addr_differ checks to manage cmov and its kin*/
BOOL do_reg_addr_read_differ_cmov_2byte(UINT32 addr, REG reg, UINT32 eflags,
					UINT32 mask, UINT32 condition)
{
    int r1, r2, r3;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    /*generic condition check */
    eflags = eflags & mask;
    r3 = (eflags == condition);
    r1 = r1 & r3;

    DPRINT(logfd, "do_reg_addr_read_differ_cmov_2byte returns %d\n", r1);
    return (r1); 
}

BOOL do_reg_addr_read_differ_cmov_4byte(UINT32 addr, REG reg, UINT32 eflags,
					UINT32 mask, UINT32 condition)
{
    int r1, r2, r3;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    /*generic condition check */
    eflags = eflags & mask;
    r3 = (eflags == condition);
    r1 = r1 & r3;

    DPRINT(logfd, "do_reg_addr_read_differ_cmov_4byte returns %d\n", r1);
    return (r1);
}

/*we have a seperate set of reg_addr_differ checks to manage cmov and its kin*/
BOOL do_reg_addr_write_differ_cmov_2byte(UINT32 addr, REG reg, UINT32 eflags,
					 UINT32 mask, UINT32 condition)
{
    int r1, r2, r3;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    /*generic condition check */
    eflags = eflags & mask;
    r3 = (eflags == condition);
    r1 = r1 & r3;

    DPRINT(logfd, "do_reg_addr_write_differ_cmov_2byte returns %d\n", r1);
    return (r1); 
}
BOOL do_reg_addr_write_differ_cmov_4byte(UINT32 addr, REG reg, UINT32 eflags,
					 UINT32 mask, UINT32 condition)
{
    int r1, r2, r3;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    /*generic condition check */
    eflags = eflags & mask;
    r3 = (eflags == condition);
    r1 = r1 & r3;

    DPRINT(logfd, "do_reg_addr_write_differ_cmov_4byte returns %d\n", r1);
    return (r1);
}


static inline BOOL cmovbe_true(UINT32 eflags)
{
    INT32 r1, r2, r3;

   /*Are either the carry flag or zero flag set?*/ 
    r1 = eflags & CF_MASK;
    r2 = (r1 == CF_MASK);
    r1 = eflags & ZF_MASK;
    r3 = (r1 == ZF_MASK);
    r3 = r3 | r2;

    return r3;
}

static inline BOOL cmovl_true(UINT32 eflags)
{
    INT32 r1, r2, r3;

   /*Do the overflow and sign flags differ? */
    r1 = eflags & SF_MASK;
    r2 = r1 >> 7;
    r1 = eflags & OF_MASK;
    r3 = r1 >> 11;
    r3 = (r2 ^ r3);

    return r3;
}

static inline BOOL cmovle_true(UINT32 eflags)
{
    INT32 r1, r2, r3;

    /*do the sign flag and overflow flag differ OR is the zero flag set?? */
    r1 = cmovl_true(eflags);
    
    r2 = eflags & ZF_MASK;
    r3 = (r2 == ZF_MASK);
    r3 = r3 | r1;

    return r3;
}

static inline BOOL cmovnle_true(UINT32 eflags)
{
    INT32 r1, r2, r3;

    /*do the sign flag and overflow flag match AND is the zero flag 0?? */
    r1 = !cmovl_true(eflags);
    
    r2 = eflags & ZF_MASK;
    r3 = (r2 == 0);
    r3 = r3 & r1;
    
    return r3;
}

BOOL do_reg_addr_read_differ_cmovbe_2byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovbe_true(eflags);
 
    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_read_differ_cmovbe_2byte returns %d\n", r1);
    return (r1); 
}

BOOL do_reg_addr_read_differ_cmovbe_4byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovbe_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_read_differ_cmovbe_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_write_differ_cmovbe_2byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovbe_true(eflags);
 
    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_write_differ_cmovbe_2byte returns %d\n", r1);
    return (r1); 
}

BOOL do_reg_addr_write_differ_cmovbe_4byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */    
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovbe_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_write_differ_cmovbe_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_read_differ_cmovl_2byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovl_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_read_differ_cmovl_2byte returns %d\n", r1);
    return (r1); 
}

BOOL do_reg_addr_read_differ_cmovl_4byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovl_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_read_differ_cmovl_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_write_differ_cmovl_2byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovl_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_write_differ_cmovl_2byte returns %d\n", r1);
    return (r1); 
}

BOOL do_reg_addr_write_differ_cmovl_4byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovl_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_write_differ_cmovl_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_read_differ_cmovnl_2byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    /*Do the overflow and sign flags match? */
    r2 = !cmovl_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_read_differ_cmovnl_2byte returns %d\n", r1);
    return (r1); 
}

BOOL do_reg_addr_read_differ_cmovnl_4byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    r2 = !cmovl_true(eflags);
    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_read_differ_cmovnl_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_write_differ_cmovnl_2byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    /*Do the overflow and sign flags match? */
    r2 = !cmovl_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_write_differ_cmovnl_2byte returns %d\n", r1);
    return (r1); 
}

BOOL do_reg_addr_write_differ_cmovnl_4byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    r2 = !cmovl_true(eflags);
    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_differ_cmovnl_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_read_differ_cmovle_2byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovle_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_read_differ_cmovbe_2byte returns %d\n", r1);
    return (r1); 
}

BOOL do_reg_addr_read_differ_cmovle_4byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    /*do the sign flag and overflow flag differ OR is the zero flag set?? */
    r2 = cmovle_true(eflags);
  
    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_read_differ_cmovbe_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_write_differ_cmovle_2byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovle_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_write_differ_cmovbe_2byte returns %d\n", r1);
    return (r1); 
}

BOOL do_reg_addr_write_differ_cmovle_4byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    /*do the sign flag and overflow flag differ OR is the zero flag set?? */
    r2 = cmovle_true(eflags);
  
    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_differ_cmovbe_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_read_differ_cmovnle_2byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovnle_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_read_differ_cmovbe_2byte returns %d\n", r1);
    return (r1); 
}

BOOL do_reg_addr_read_differ_cmovnle_4byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    /*do the sign flag and overflow flag match AND is the zero flag 0?? */
    r2 = cmovnle_true(eflags);
    
    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_read_differ_cmovbe_4byte returns %d\n", r1);
    return (r1);
}

BOOL do_reg_addr_write_differ_cmovnle_2byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_2byte(addr);
    r1 = r1 ^ r2;

    r2 = cmovnle_true(eflags);

    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_write_differ_cmovbe_2byte returns %d\n", r1);
    return (r1); 
}

BOOL do_reg_addr_write_differ_cmovnle_4byte(UINT32 addr, REG reg, UINT32 eflags)
{
    int r1, r2;

    /*do the taint values of the reg and addr location differ? */
    r1 = global_regs[reg];
    r2 = addr_tainted_4byte(addr);
    r1 = r1 ^ r2;

    /*do the sign flag and overflow flag match AND is the zero flag 0?? */
    r2 = cmovnle_true(eflags);
    
    r1 = r1 & r2;

    DPRINT(logfd, "do_reg_addr_write_differ_cmovbe_4byte returns %d\n", r1);
    return (r1);
}

static inline BOOL reg_differ(REG reg1, REG reg2)
{
    INT32 r1, r2;

    r1 = global_regs[reg1];
    r2 = global_regs[reg2];

    r1 = r1 ^ r2;

    return r1;
}

BOOL do_reg_differ_cmov(REG reg1, REG reg2, UINT32 eflags, UINT32 mask, UINT32 condition)
{
    INT32 r1, r2;

    r1 = reg_differ(reg1, reg2);

    /*generic condition check */
    eflags = eflags & mask;
    r2 = (eflags == condition);
    r1 = r1 & r2;

    return r1;
}

BOOL do_reg_differ_cmovbe(REG reg1, REG reg2, UINT32 eflags)
{
    INT32 r1, r2;
    
    r1 = reg_differ(reg1, reg2);
    r2 = cmovbe_true(eflags);

    r1 = r1 & r2;

    return r1;
}

BOOL do_reg_differ_cmovl(REG reg1, REG reg2, UINT32 eflags)
{
    INT32 r1, r2;
    
    r1 = reg_differ(reg1, reg2);
    r2 = cmovl_true(eflags);

    r1 = r1 & r2;

    return r1;
}

BOOL do_reg_differ_cmovnl(REG reg1, REG reg2, UINT32 eflags)
{
    INT32 r1, r2;
    
    r1 = reg_differ(reg1, reg2);
    r2 = !cmovl_true(eflags);

    r1 = r1 & r2;

    return r1;
}

BOOL do_reg_differ_cmovle(REG reg1, REG reg2, UINT32 eflags)
{
    INT32 r1, r2;
    
    r1 = reg_differ(reg1, reg2);
    r2 = cmovle_true(eflags);

    r1 = r1 & r2;

    return r1;
}

BOOL do_reg_differ_cmovnle(REG reg1, REG reg2, UINT32 eflags)
{
    INT32 r1, r2;
    
    r1 = reg_differ(reg1, reg2);
    r2 = cmovnle_true(eflags);

    r1 = r1 & r2;

    return r1;
}
/*sigh: cmov ins are not branch points.  The only way to
  tell if the move happens is to check the condition (flags register) myself
*/
INT32 cmov_flags(INS ins, OPCODE opcode, UINT32* mask, UINT32* condition)
{
    INT32 rc = 0;

    switch(opcode) {
    case XED_ICLASS_CMOVO:
	//OF == 1
	*mask = OF_MASK;
	*condition = OF_MASK;
	rc = CMOV_GENERIC;
	break;
    case XED_ICLASS_CMOVNO:
	//OF == 0
	*mask = OF_MASK;
	*condition = 0;
	rc = CMOV_GENERIC;
	break;
    case XED_ICLASS_CMOVB:
	//CF == 1
	*mask = CF_MASK;
	*condition = CF_MASK;
	rc = CMOV_GENERIC;
	break;
    case XED_ICLASS_CMOVNB:
	//CF == 0
	*mask = CF_MASK;
	*condition = 0;
	rc = CMOV_GENERIC;
	break;
    case XED_ICLASS_CMOVZ:
	//ZF == 1
	*mask = ZF_MASK;
	*condition = ZF_MASK;
	rc = CMOV_GENERIC;
	break;
    case XED_ICLASS_CMOVNZ:
	//ZF == 0
	*mask = ZF_MASK;
	*condition = 0;
	rc = CMOV_GENERIC;
	break;
    case XED_ICLASS_CMOVBE:
	//CF == 1 || ZF == 1
	rc = CMOVBE;
	break;
    case XED_ICLASS_CMOVNBE:
	//CF == 0 && ZF == 0
	*mask = (CF_MASK | ZF_MASK);
	*condition = 0;
	rc = CMOV_GENERIC;
	break;
    case XED_ICLASS_CMOVS:
	//SF == 1
	*mask = SF_MASK;
	*condition = SF_MASK;
	rc = CMOV_GENERIC;
	break;
    case XED_ICLASS_CMOVNS:
	//SF == 0
	*mask = SF_MASK;
	*condition = 0;
	rc = CMOV_GENERIC;
	break;
    case XED_ICLASS_CMOVP:
	//PF == 1
	*mask = PF_MASK;
	*condition = PF_MASK;
	rc = CMOV_GENERIC;
	break;
    case XED_ICLASS_CMOVNP:
	//PF == 0
	*mask = PF_MASK;
	*condition = 0;
	rc = CMOV_GENERIC;
	break;
    case XED_ICLASS_CMOVL:
	//SF != OF
	rc = CMOVL;
	break;
    case XED_ICLASS_CMOVNL:
	//SF == OF
	rc = CMOVNL;
	break;
    case XED_ICLASS_CMOVLE:
	//ZF == 1 || SF != OF
	rc = CMOVLE;
	break;
    case XED_ICLASS_CMOVNLE:
	//ZF == 0 && SF == OF
	rc = CMOVNLE;
	break;
    }

    return rc;
}

void cmov_memread(INS ins, INT32 cmov_type, USIZE addrsize, UINT32 mask, UINT32 condition,
		  REG reg)
{

    switch(cmov_type) {
    case CMOV_GENERIC:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmov_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_UINT32, mask, IARG_UINT32, condition, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_2byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END); 
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmov_4byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS, 
			     IARG_UINT32, mask, IARG_UINT32, condition, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_4byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg,  IARG_END); 
	    break;
	}
	break;
    case CMOVBE:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmovbe_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS, 
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_2byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmovbe_4byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS, 
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_4byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END); 
	    break;
	}
	break;	    
    case CMOVL:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmovl_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS, 
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_2byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
			      
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmovl_4byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS, 
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_4byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	}
	break;	    
    case CMOVNL:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmovnl_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS, 
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_2byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmovnl_4byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS, 
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_4byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	}
	break;	    
    case CMOVLE:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmovle_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS, 
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_2byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmovle_4byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS, 
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_4byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	}
	break;	    
    case CMOVNLE:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmovnle_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS, 
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_2byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_read_differ_cmovnle_4byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS, 
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_4byte),
			       IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	    break;
	}
	break;	    
    }
}

void cmov_memwrite(INS ins, INT32 cmov_type, USIZE addrsize, UINT32 mask, UINT32 condition,
		  REG reg)
{
    switch(cmov_type) {
    case CMOV_GENERIC:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmov_2byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_UINT32, mask, IARG_UINT32, condition, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_2byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmov_4byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_UINT32, mask, IARG_UINT32, condition, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	}
	break;
    case CMOVBE:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmovbe_2byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_2byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmovbe_4byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	}
	break;	    
    case CMOVL:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmovl_2byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_2byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmovl_4byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	}
	break;	    
    case CMOVNL:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmovnl_2byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmovnl_4byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	}
	break;	    
    case CMOVLE:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmovle_2byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_2byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmovle_4byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	}
	break;	    
    case CMOVNLE:
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmovnle_2byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_2byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_write_differ_cmovnle_4byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_REG_VALUE, REG_EFLAGS,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	}
	break;	    
    }
}

void cmov_regmov(INS ins, INT32 cmov_type, USIZE addrsize, UINT32 mask, UINT32 condition,
		 REG dstreg, REG srcreg)
{
    switch(cmov_type) {
    case CMOV_GENERIC:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_differ_cmov),
			 IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_REG_VALUE, REG_EFLAGS,
			 IARG_UINT32, mask, IARG_UINT32, condition, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(match_regs),
			   IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_END);
	break;
    case CMOVBE:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_differ_cmovbe),
			 IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_REG_VALUE, REG_EFLAGS,
			 IARG_END); 
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(match_regs),
			   IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_END);
	break;	    
    case CMOVL:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_differ_cmovl),
			 IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_REG_VALUE, REG_EFLAGS,
			 IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(match_regs),
			   IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_END);
	
	break;	    
    case CMOVNL:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_differ_cmovnl),
			 IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_REG_VALUE, REG_EFLAGS,
			 IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(match_regs),
			   IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_END);
	break;	    
    case CMOVLE:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_differ_cmovle),
			 IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_REG_VALUE, REG_EFLAGS,
			 IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(match_regs),
			   IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_END);
	break;	    
    case CMOVNLE:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_differ_cmovnle),
			 IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_REG_VALUE, REG_EFLAGS,
			 IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(match_regs),
			   IARG_UINT32, dstreg, IARG_UINT32, srcreg, IARG_END);
	break;	    
    }
}


void instrument_cmov(INS ins, OPCODE opcode)
{
    int ismemread = 0;
    int ismemwrite = 0;
    USIZE addrsize = 0;
    UINT32 mask = 0;
    UINT32 condition = 0;
    REG reg;
    REG dstreg;
    INT32 cmov_type;

    reg = (REG) 0;
    dstreg = (REG) 0;

    //if one operand is memory the other must be a register for cmov
    if(INS_IsMemoryRead(ins)) {
	ismemread = 1;
	addrsize = INS_MemoryReadSize(ins);
	reg = INS_OperandReg(ins, 0);
	if(!is_valid_reg(reg)) return;
    } else if(INS_IsMemoryWrite(ins)) {
	ismemwrite = 1;
	addrsize = INS_MemoryWriteSize(ins);
	reg = INS_OperandReg(ins, 1);
	if(!is_valid_reg(reg)) return;
    } 

    cmov_type = cmov_flags(ins, opcode, &mask, &condition);

    if(ismemread) {
	cmov_memread(ins, cmov_type, addrsize,mask, condition, reg);
    } else if(ismemwrite) {
	cmov_memwrite(ins, cmov_type, addrsize, mask, condition, reg);
    } else {
	//sometimes get an offset into video memory
	if(!(INS_OperandIsReg(ins, 0) && INS_OperandIsReg(ins, 1))) return;
	dstreg = INS_OperandReg(ins, 0);
	reg = INS_OperandReg(ins, 1);
	if(!is_valid_reg(dstreg) || !is_valid_reg(reg)) return;
	cmov_regmov(ins, cmov_type, addrsize, mask, condition, reg, dstreg);
    }
    
}

void instrument_move_string(INS ins, OPCODE opcode)
{
    if(INS_RepPrefix(ins)) {
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2mem_generic),
		       IARG_MEMORYREAD_EA, IARG_MEMORYWRITE_EA,
		       IARG_MEMORYWRITE_SIZE, IARG_END);	
    }


    switch(opcode) {
    case XED_ICLASS_MOVSB:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_addr_differ_1byte),
			 IARG_MEMORYREAD_EA, IARG_MEMORYWRITE_EA, IARG_END);

	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2mem_1byte),
			   IARG_MEMORYREAD_EA, IARG_MEMORYWRITE_EA, IARG_END);
	break;
    case XED_ICLASS_MOVSW:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_addr_differ_2byte),
			 IARG_MEMORYREAD_EA, IARG_MEMORYWRITE_EA, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2mem_2byte),
			   IARG_MEMORYREAD_EA, IARG_MEMORYWRITE_EA, IARG_END);
	break;
    case XED_ICLASS_MOVSD:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_addr_differ_4byte),
			 IARG_MEMORYREAD_EA, IARG_MEMORYWRITE_EA, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2mem_4byte),
			   IARG_MEMORYREAD_EA, IARG_MEMORYWRITE_EA, IARG_END);
	break;
    case XED_ICLASS_MOVSQ:
	fprintf(errfd, "got a 64 bit movsq!!!\n");
	break;
//    case XED_ICLASS_MOVS:
//	break;
    }
}

void instrument_load_string(INS ins, OPCODE opcode)
{
    REG reg;

    reg = INS_OperandReg(ins, 0);

    DPRINT(stderr, "instrument_load_string\n");
    switch(opcode) {
    case XED_ICLASS_LODSB:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_1byte),
			 IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_1byte),
			   IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	break;
    case XED_ICLASS_LODSW:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_2byte),
			 IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_2byte),
			   IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	break;
    case XED_ICLASS_LODSD:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_4byte),
			 IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_4byte),
			   IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	break;
    case XED_ICLASS_LODSQ:
	fprintf(errfd, "got a 64 bit lodsq!!!\n");
	break;
    }

}

void instrument_store_string(INS ins, OPCODE opcode)
{
    REG reg;

    reg = INS_OperandReg(ins, 2);
    DPRINT(stderr, "instrument_store_string\n");

    if(INS_RepPrefix(ins)) {
	INS_InsertCall(ins, IPOINT_BEFORE, 
		       AFUNPTR(cp_taint_reg2mem_generic),
		       IARG_UINT32, reg, IARG_MEMORYWRITE_EA,
		       IARG_MEMORYWRITE_SIZE, IARG_END);
	return;
    }

    switch(opcode) {
    case XED_ICLASS_STOSB:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_1byte),
			 IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_1byte),
			   IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	break;
    case XED_ICLASS_STOSW:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_2byte),
			 IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_2byte),
			   IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	break;
    case XED_ICLASS_STOSD:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_4byte),
			 IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
			   IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	break;
    case XED_ICLASS_STOSQ:
	fprintf(errfd, "got a 64 bit stosq!!!\n");
	break;
    }
}

void instrument_push(INS ins)
{
    REG reg = REG_INVALID();
    USIZE addrsize = INS_MemoryWriteSize(ins);
    
    //pin push format src reg, mem write loc, stackpointer
    if(!INS_OperandIsReg(ins, 0)) {
	//pushing an imm value onto the stack
	switch(addrsize) {
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_2byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, IMMEDIATE_VAL, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_2byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, IMMEDIATE_VAL, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_4byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, IMMEDIATE_VAL, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, IMMEDIATE_VAL, IARG_END);
	    break;
	default:
	    fprintf(errfd, "instrument_push unknown addr size is %d\n", addrsize); 
	}
    } else {
	reg = INS_OperandReg(ins, 0);
	switch(addrsize) { 
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_2byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_2byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_4byte),
			     IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_reg2mem_4byte),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, reg, IARG_END);
	    break;
	default:
	    fprintf(errfd, "instrument_push unknown addr size is %d\n", addrsize);
	}
    }
}


void instrument_pop(INS ins)
{
    REG reg = REG_INVALID();
    USIZE addrsize = INS_MemoryReadSize(ins);

    //pin pop format dest reg, mem read loc, stackpointer
    reg = INS_OperandReg(ins, 0);
    DPRINT(logfd, "instrument pop: reg is num %d size of mem read is %u\n", 
	    reg, addrsize);
    switch(addrsize) {
    case 2:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_2byte),
			 IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_UINT32, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_2byte),
			   IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	break;
    case 4:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(do_reg_addr_differ_4byte),
			 IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(cp_taint_mem2reg_4byte),
			   IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_END);
	break;
    default:
	fprintf(errfd, "instrument_pop unknown addr size is %d\n", addrsize);	
    }
}

void instrument_addorsub(INS ins)
{
    int op1mem, op2mem, op1reg, op2reg, op2imm;
    string instruction;
    int count;
    OPCODE opcode;
    USIZE addrsize;
    REG reg;

    count = INS_OperandCount(ins);
    opcode = INS_Opcode(ins);
    DPRINT(logfd, "arith opcode is %u\n", opcode);
    DPRINT(logfd, "arith instruction num %d has %d operands\n", opcode, count);

    op1mem = INS_OperandIsMemory(ins, 0);
    op2mem = INS_OperandIsMemory(ins, 1);
    op1reg = INS_OperandIsReg(ins, 0);
    op2reg = INS_OperandIsReg(ins, 1);
    op2imm = INS_OperandIsImmediate(ins, 1);

    if((op1mem && op2reg)) {
	reg = INS_OperandReg(ins, 1);
	if(!is_valid_reg(reg)) {
	    return;
	}
	addrsize = INS_MemoryWriteSize(ins);

	switch(addrsize) { 
	case 1:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_reg_tainted),
			     IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_addr_1byte),
			       IARG_MEMORYWRITE_EA, IARG_END);
	    break;
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_reg_tainted),
			     IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_addr_2byte),
			       IARG_MEMORYWRITE_EA, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_reg_tainted),
			     IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_addr_4byte),
			       IARG_MEMORYWRITE_EA, IARG_END);
	    break;
	case 8:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_reg_tainted),
			     IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_addr_generic),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, 8, IARG_END);
	    break;
	case 16:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_reg_tainted),
			     IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_addr_generic),
			       IARG_MEMORYWRITE_EA, IARG_UINT32, 16, IARG_END);
	    break;

	default:
	    fprintf(logfd, "got unknown addr size for add/sub size %d\n", addrsize);
#if 0
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_tainted),
			     IARG_UINT32, reg, IARG_UINT32, REG_READ, 
			     IARG_UINT32, base, IARG_UINT32, REG_READ, 
			     IARG_UINT32, index, IARG_UINT32, REG_READ, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_addr_generic),
			       IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, 
			       IARG_END);
#endif
	}
    } else if(op1reg && op2mem) {
	DPRINT(logfd, "op1 is reg && op2 is mem\n");
	reg = INS_OperandReg(ins, 0);
	if(!INS_IsMemoryRead(ins) || !is_valid_reg(reg)) {
	    //we ignore reads from video memory e.g. the gs segment register
	    return;
	}
	addrsize = INS_MemoryReadSize(ins);
	switch(addrsize) {
	case 1:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_addr_tainted_1byte),
			     IARG_MEMORYREAD_EA, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_reg),
			       IARG_UINT32, reg, IARG_END);
	    break;
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_addr_tainted_2byte),
			     IARG_MEMORYREAD_EA, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_reg),
			       IARG_UINT32, reg, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_addr_tainted_4byte),
			     IARG_MEMORYREAD_EA, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_reg),
			       IARG_UINT32, reg, IARG_END);
	    break;
	case 8:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_addr_tainted_8byte),
			     IARG_MEMORYREAD_EA, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_reg),
			       IARG_UINT32, reg, IARG_END);
	    break;
	case 16:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_addr_tainted_16byte),
			     IARG_MEMORYREAD_EA, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_reg),
			       IARG_UINT32, reg, IARG_END);
	    break;
	default:
	    fprintf(logfd, "got unknown addr size for add/sub size %d\n", addrsize);
	}
    } else if(op1reg && op2reg) {
	REG dstreg;
	DPRINT(logfd, "op1 and op2 of Artith are registers\n");
	dstreg = INS_OperandReg(ins, 0);
	reg = INS_OperandReg(ins, 1);
	if(!is_valid_reg(dstreg) || !is_valid_reg(reg)) {
	    return;
	} 
	if((opcode == XED_ICLASS_XOR || opcode == XED_ICLASS_SUB || 
	    opcode == XED_ICLASS_SBB || opcode == XED_ICLASS_PXOR ||
	    opcode == XED_ICLASS_FSUB || opcode == XED_ICLASS_FSUBP ||
	    opcode == XED_ICLASS_FSUBP || opcode == XED_ICLASS_FISUB ||
	    opcode == XED_ICLASS_FSUBR || opcode == XED_ICLASS_FISUBR ||
	    opcode == XED_ICLASS_FSUBRP || opcode == XED_ICLASS_XORPS ||
	    opcode == XED_ICLASS_PSUBB || opcode == XED_ICLASS_PSUBW ||
	    opcode == XED_ICLASS_PSUBD || opcode == XED_ICLASS_PSUBQ) 
	   && (dstreg == reg)) {
	    DPRINT(logfd, "handling reg reset\n");
	    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(clear_reg),
			   IARG_UINT32, dstreg, IARG_END);
	} else {
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_reg_tainted),
			     IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_reg),
			       IARG_UINT32, dstreg, IARG_END);
	}
    } else if(op1mem && op2imm) {
	/*imm does not change taint value of the destination*/
	return;
    }else if(!(op1reg && op2imm)){
	//if the arithmatic involves an immediate instruction the taint does
	//not propagate...
	string instruction;
	instruction = INS_Disassemble(ins);
	fprintf(logfd, "unknown combination of arithmatic ins: %s\n", instruction.c_str());
    }
}

/*mul can have 1, 2, or 3 operands results are always stored in a register.*/
/*div has 3 operands and the results are always stored in a register*/
void instrument_mulordiv(INS ins)
{
    int op1reg, op2reg, op3reg;
    string instruction;
    int count;
    OPCODE opcode;
    int memread;
    UINT32 reg1stat, reg2stat, reg3stat;
    REG reg1, reg2, reg3;
    USIZE addrsize;

    count = INS_OperandCount(ins);
    opcode = INS_Opcode(ins);
    DPRINT(logfd, "mul opcode is %u\n", opcode);
    DPRINT(logfd, "mul instruction num %d has %d operands\n", opcode, count);

    reg1 = REG_INVALID();
    reg2 = REG_INVALID();
    reg3 = REG_INVALID();

    reg1stat = 0;
    reg2stat = 0;
    reg3stat = 0;

    memread = INS_IsMemoryRead(ins);
    op1reg = INS_OperandIsReg(ins, 0);
    op2reg = INS_OperandIsReg(ins, 1);
    op3reg = 0;
    if(count == 4) {
	//4th operand is proc flag that we ignore
	op3reg = INS_OperandIsReg(ins, 2);
    }
    if(op1reg && REG_valid(reg1) && is_valid_reg(reg1)) {
	reg1 = INS_OperandReg(ins, 0);
	if(INS_RegRContain(ins, reg1)) {
	    reg1stat |= REG_READ;
	}
	if(INS_RegWContain(ins, reg1)) {
	    reg1stat |= REG_WRITE;
	}
    }
    if(op2reg && REG_valid(reg2) && is_valid_reg(reg2)) {
	reg2 = INS_OperandReg(ins, 1);
	if(INS_RegRContain(ins, reg2)) {
	    reg2stat |= REG_READ;
	}
	if(INS_RegWContain(ins, reg2)) {
	    reg2stat |= REG_WRITE;
	}
    }
    if(op3reg && REG_valid(reg3) && is_valid_reg(reg3)) {
	reg3 = INS_OperandReg(ins, 2);
	if(INS_RegRContain(ins, reg3)) {
	    reg3stat |= REG_READ;
	}
	if(INS_RegWContain(ins, reg3)) {
	    reg3stat |= REG_WRITE;
	}
    }
    
    if(memread) {
	//memlocation cannot be destination for mul or divide
	addrsize = INS_MemoryReadSize(ins);

	switch(addrsize) {
	case 1:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_1byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg1, 
			     IARG_UINT32, reg1stat, 
			     IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
			     IARG_UINT32, reg3, IARG_UINT32, reg3stat,IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_dest_regs),
			       IARG_UINT32, reg1, IARG_UINT32, reg1stat, 
			       IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
			       IARG_UINT32, reg3, IARG_UINT32, reg3stat,
			       IARG_END);
	    break;
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg1, 
			     IARG_UINT32, reg1stat, 
			     IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
			     IARG_UINT32, reg3, IARG_UINT32, reg3stat, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_dest_regs),
			       IARG_UINT32, reg1, IARG_UINT32, reg1stat, 
			       IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
			       IARG_UINT32, reg3, IARG_UINT32, reg3stat,
			       IARG_END);
	    break;
	case 4:
	     INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_4byte),
			      IARG_MEMORYREAD_EA, IARG_UINT32, reg1, 
			      IARG_UINT32, reg1stat, 
			      IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
			      IARG_UINT32, reg3, IARG_UINT32, reg3stat, IARG_END);
	     INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_dest_regs),
				IARG_UINT32, reg1, IARG_UINT32, reg1stat, 
				IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
				IARG_UINT32, reg3, IARG_UINT32, reg3stat,
				IARG_END);
	     break;
	case 8:
	     INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_8byte),
			      IARG_MEMORYREAD_EA, IARG_UINT32, reg1, 
			      IARG_UINT32, reg1stat, 
			      IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
			      IARG_UINT32, reg3, IARG_UINT32, reg3stat, IARG_END);
	     INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_dest_regs),
				IARG_UINT32, reg1, IARG_UINT32, reg1stat, 
				IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
				IARG_UINT32, reg3, IARG_UINT32, reg3stat,
				IARG_END);
	     break;
	case 16:
	     INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_16byte),
			      IARG_MEMORYREAD_EA, IARG_UINT32, reg1, 
			      IARG_UINT32, reg1stat, 
			      IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
			      IARG_UINT32, reg3, IARG_UINT32, reg3stat, IARG_END);
	     INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_dest_regs),
				IARG_UINT32, reg1, IARG_UINT32, reg1stat, 
				IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
				IARG_UINT32, reg3, IARG_UINT32, reg3stat,
				IARG_END);
	     break;
	default:
	    fprintf(logfd, "Mul or divide got unexpected memread size %d\n", addrsize);
	}
    } else {
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_tainted),
			 IARG_UINT32, reg1, IARG_UINT32, reg1stat, 
			 IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
			 IARG_UINT32, reg3, IARG_UINT32, reg3stat,
			 IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(taint_dest_regs),
			   IARG_UINT32, reg1, IARG_UINT32, reg1stat, 
			   IARG_UINT32, reg2, IARG_UINT32, reg2stat, 
			   IARG_UINT32, reg3, IARG_UINT32, reg3stat,
			   IARG_END);
    }
}

void instrument_xchg(INS ins)
{
    int op1mem, op2mem, op1reg, op2reg;
    USIZE addrsize;

    op1mem = INS_OperandIsMemory(ins, 0);
    op2mem = INS_OperandIsMemory(ins, 1);
    op1reg = INS_OperandIsReg(ins, 0);
    op2reg = INS_OperandIsReg(ins, 1);

    if(op1reg && op2reg) {
	REG reg1, reg2;
	DPRINT(logfd, "op1 and op2 of Artith are registers\n");
	reg1 = INS_OperandReg(ins, 0);
	reg2 = INS_OperandReg(ins, 1);
	if(!is_valid_reg(reg1) || !is_valid_reg(reg2)) {
	    return;
	}

	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(xchg_regs),
		       IARG_UINT32, reg1, IARG_UINT32, reg2, IARG_END);
    } else if(op1reg && op2mem) {
	REG reg;
	DPRINT(logfd, "op1 is reg && op2 is mem\n");
	addrsize = INS_MemoryReadSize(ins);
	reg = INS_OperandReg(ins, 0);
	if(!is_valid_reg(reg)) {
	    //we ignore reads from video memory e.g. the gs segment register
	    return;
	}
	switch(addrsize) {
	case 1:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_1byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_UINT32, REG_READ,
			     IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(xchg_reg_mem_1byte),
			       IARG_UINT32, reg, IARG_MEMORYREAD_EA, IARG_END);
	    break;
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_UINT32, REG_READ,
			     IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(xchg_reg_mem_2byte),
			       IARG_UINT32, reg, IARG_MEMORYREAD_EA, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_4byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_UINT32, REG_READ,
			     IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(xchg_reg_mem_4byte),
			       IARG_UINT32, reg, IARG_MEMORYREAD_EA, IARG_END);
	    break;
	default:
	    fprintf(logfd, "got unexpected address size for xchg size is %d\n", addrsize);
	}
    } else if(op1mem && op2reg) {
	REG reg;
	DPRINT(logfd, "op1 is mem && op2 is reg\n");
	addrsize = INS_MemoryReadSize(ins);
	reg = INS_OperandReg(ins, 1);
	if(!is_valid_reg(reg)) {
	    //we ignore reads from video memory e.g. the gs segment register
	    return;
	}
	switch(addrsize) {
	case 1:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_1byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_UINT32, REG_READ,
			     IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(xchg_reg_mem_1byte),
			       IARG_UINT32, reg, IARG_MEMORYREAD_EA, IARG_END);
	    break;
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_UINT32, REG_READ,
			     IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(xchg_reg_mem_2byte),
			       IARG_UINT32, reg, IARG_MEMORYREAD_EA, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_4byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_UINT32, REG_READ,
			     IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(xchg_reg_mem_4byte),
			       IARG_UINT32, reg, IARG_MEMORYREAD_EA, IARG_END);
	    break;
	default:
	    fprintf(logfd, "got unexpected address size for xchg size is %d\n", addrsize);
	}
    } else {
	string instruction;
	instruction = INS_Disassemble(ins);
	fprintf(logfd, "unknown combination of XCHG ins: %s\n", instruction.c_str());
	breakdown_ins(ins);
    }
}

void instrument_xadd(INS ins)
{
    int op2mem, op1reg, op2reg;
    USIZE addrsize;

    op2mem = INS_OperandIsMemory(ins, 1);
    op1reg = INS_OperandIsReg(ins, 0);
    op2reg = INS_OperandIsReg(ins, 1);

    if(op1reg && op2reg) {
	REG reg1, reg2;
	DPRINT(logfd, "op1 and op2 of Artith are registers\n");
	reg1 = INS_OperandReg(ins, 0);
	reg2 = INS_OperandReg(ins, 1);
	if(!is_valid_reg(reg1) || !is_valid_reg(reg2)) {
	    return;
	}
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(xadd_regs),
		       IARG_UINT32, reg1, IARG_UINT32, 
		       reg2, IARG_END);
    } else if(op1reg && op2mem) {
	REG reg;
	DPRINT(logfd, "op1 is reg && op2 is mem\n");
	addrsize = INS_MemoryReadSize(ins);
	reg = INS_OperandReg(ins, 0);
	if(!is_valid_reg(reg)) {
	    //we ignore reads from video memory e.g. the gs segment register
	    return;
	}
	switch(addrsize) {
	case 1:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_1byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_UINT32, REG_READ,
			     IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(xadd_reg_mem_1byte),
			       IARG_UINT32, reg, IARG_MEMORYREAD_EA, IARG_END);
	    break;
	case 2:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_2byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_UINT32, REG_READ,
			     IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(xadd_reg_mem_2byte),
			       IARG_UINT32, reg, IARG_MEMORYREAD_EA, IARG_END);
	    break;
	case 4:
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(are_regs_mem_read_tainted_4byte),
			     IARG_MEMORYREAD_EA, IARG_UINT32, reg, IARG_UINT32, REG_READ,
			     IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0, IARG_UINT32, 0,
			     IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(xadd_reg_mem_4byte),
			       IARG_UINT32, reg, IARG_MEMORYREAD_EA, IARG_END);
	    break;
	default:
	    fprintf(logfd, "got unexpected address size for xadd size is %d\n", addrsize);
	}
    } else {
	string instruction;
	instruction = INS_Disassemble(ins);
	fprintf(logfd, "unknown combination of XADD ins: %s\n", instruction.c_str());
    }    
}

void instrument_lea(INS ins)
{
    REG dstreg = REG_INVALID();

    dstreg = INS_OperandReg(ins, 0);

    //loading an effective address clears the dep set of the target
    //register.. then we add the depsets of the base/index regs used.
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(clear_reg),
		   IARG_UINT32, dstreg, IARG_END);
}

//i think we can optimize the number of IF/THEN insert calls here
void instrument_syscall_entry(THREADID tid, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
#if 0
    fprintf(stderr, "instrument_syscall_entry: syscall is %d\n", PIN_GetSyscallNumber(ctxt, std));
#endif
    //read
    check_read_syscall(PIN_GetSyscallNumber(ctxt, std), PIN_GetSyscallArgument(ctxt, std, 1)); 

    //recv, recvfrom etc.
    if (PIN_GetSyscallNumber(ctxt, std) == SYS_socketcall)
        get_socketcall_args(PIN_GetSyscallArgument(ctxt, std, 0), PIN_GetSyscallArgument(ctxt, std, 1));
    else
    	valid_socketcall = 0;
	// jlgreath jlgreath jlgreath
}

void instrument_syscall_exit(THREADID tid, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
#if 0
	fprintf(stderr, "syscall exit\n");
#endif
	if (valid_readcall == 1) {
		taint_read_result(PIN_GetSyscallReturn(ctxt, std));
    }
    if (valid_socketcall == 1) {
    	handle_socketcall_result(PIN_GetSyscallReturn(ctxt, std));
    }
}

void print_return_addr(ADDRINT addr)
{
    fprintf(logfd, "about to check return addr 0x%08x\n", addr);
}

void instrument_ret(INS ins)
{
    USIZE addrsize = INS_MemoryReadSize(ins);
#if TAINT_DEBUG 
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(print_return_addr),
		   IARG_MEMORYREAD_EA, IARG_END);
#endif
    switch(addrsize) {
    case 2:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_addr_tainted_2byte),
			 IARG_MEMORYREAD_EA, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(compromised),
			   IARG_MEMORYREAD_EA, IARG_UINT32, 2, IARG_END);
	break;
    case 4:
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_addr_tainted_4byte),
			 IARG_MEMORYREAD_EA, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(compromised),
			   IARG_MEMORYREAD_EA, IARG_UINT32, 4, IARG_END);
	break;
    default:
	fprintf(errfd, "unknown addr size in instrument return %u\n", 
		addrsize);
    }
}

void print_direct(ADDRINT addr)
{
    fprintf(logfd, "about to check direct call addr 0x%08x\n", addr);
}

void print_indirect_addr(ADDRINT addr)
{
    fprintf(logfd, "about to check indirect call addr 0x%08x\n", addr);
}

void print_indirect_reg(UINT32 reg)
{
    fprintf(logfd, "about to check indirect reg %d\n", reg);
}

void print_call_write_addr(UINT32 addr, UINT32 numbytes)
{
    fprintf(logfd, "call writing to address 0x%08x numbytes %u\n", addr, numbytes);
}

void instrument_call(INS ins)
{
    USIZE addrsize;
    //    string instruction;

    //    instruction = INS_Disassemble(ins);
    //    fprintf(logfd, "CALL: %s  \n", instruction.c_str());

    //breakdown_ins(ins);
    if(INS_IsMemoryWrite(ins)) {
#if 0	
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(print_call_write_addr),
		       IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_END);
#endif
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_addr_tainted_4byte),
			 IARG_MEMORYWRITE_EA, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(clear_addr_4byte),
			   IARG_MEMORYWRITE_EA, IARG_END);
    }

    if(INS_IsIndirectBranchOrCall(ins)) {
	if(INS_IsMemoryRead(ins)) {
	    addrsize = INS_MemoryReadSize(ins);
	    DPRINT(stderr, "indirect call is memory read\t");
#if TAINT_DEBUG 
	    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(print_indirect_addr),
			   IARG_MEMORYREAD_EA, IARG_END);
#endif
	    switch(addrsize) {
	    case 2:
		INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_addr_tainted_2byte),
				 IARG_MEMORYREAD_EA, IARG_END);
		INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(compromised),
				   IARG_MEMORYREAD_EA, IARG_UINT32, 2,IARG_END);
		break;
	    case 4:
		INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_addr_tainted_4byte),
				 IARG_MEMORYREAD_EA, IARG_END);
		INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(compromised),
				   IARG_MEMORYREAD_EA, IARG_UINT32, 4, IARG_END);
		break;
	    default:
		fprintf(errfd, "unknown addr size in instrument call %u\n", addrsize);
	    }
	} else if(INS_OperandIsReg(ins, 0)) {
	    REG reg;
	    DPRINT(stderr, "indirect call is reg access\t");
	    reg = INS_OperandReg(ins, 0);
#if TAINT_DEBUG 
	    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(print_indirect_reg),
			   IARG_UINT32, reg, IARG_END);
#endif
	    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_reg_tainted),
			     IARG_UINT32, reg, IARG_END);
	    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(compromised),
			       IARG_UINT32, 0, IARG_UINT32, 0,IARG_END);
	}
    } else if(INS_IsDirectBranchOrCall(ins)) {
	ADDRINT addr;
	addr = INS_Address(ins);
	if((addr & 0xFFF) + 4 > 4096) {
	    fprintf(logfd, "instruction at addr 0x%08x spans page boundaries\n", addr);
	    return;
	}
	DPRINT(stderr, "Direct near call (imm)instruction \t");
#if TAINT_DEBUG 
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(print_direct),
		       IARG_ADDRINT, addr, IARG_END);
#endif
	INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(is_addr_tainted_4byte),
			 IARG_ADDRINT, addr, IARG_END);
	INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(compromised),
			   IARG_UINT32, addr, IARG_UINT32, 4, IARG_END);
    } else {
	fprintf(stderr, "unknown proc call ins???\n");
	breakdown_ins(ins);
    }
}

#ifndef INS_COUNT 
#define cnt_ins(...)

#else
void cnt_ins(INS ins, OPCODE opcode)
{
    INT32 category;

    category = INS_Category(ins);

    if(INS_IsSyscall(ins)) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &syscall_ins, IARG_END);
    } else if(INS_IsMov(ins) || opcode == XED_ICLASS_MOVSX ||
	      opcode == XED_ICLASS_MOVZX) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &mov_ins, IARG_END);
    } else if(category == XED_CATEGORY_CMOV) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &cmov_ins, IARG_END);
    } else if(category == XED_CATEGORY_PUSH) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &push_ins, IARG_END);
    } else if(category == XED_CATEGORY_POP) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &pop_ins, IARG_END);
    } else if(INS_IsXchg(ins)) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &xchg_ins, IARG_END);
    } else if(opcode == XED_ICLASS_ADD || opcode == XED_ICLASS_ADC ||
	      opcode == XED_ICLASS_SUB || opcode == XED_ICLASS_SBB ||
	      opcode == XED_ICLASS_OR  || opcode == XED_ICLASS_AND ||
	      opcode == XED_ICLASS_XOR) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &arith_ins, IARG_END);
    } else if(opcode == XED_ICLASS_MUL || opcode == XED_ICLASS_IMUL ||
	      opcode == XED_ICLASS_DIV || opcode == XED_ICLASS_IDIV) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &arith_ins, IARG_END);
    } else if(opcode == XED_ICLASS_XADD) {
	DPRINT(stderr, "about to instruent xadd\n");
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &xadd_ins, IARG_END);
    } else if(opcode == XED_ICLASS_MOVSB ||
	      opcode == XED_ICLASS_MOVSW || opcode == XED_ICLASS_MOVSD ||
	      opcode == XED_ICLASS_MOVSQ) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &string_ins, IARG_END);
    } else if(opcode == XED_ICLASS_STOSB ||
	      opcode == XED_ICLASS_STOSW || opcode == XED_ICLASS_STOSD ||
	      opcode == XED_ICLASS_STOSQ) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &string_ins, IARG_END);
    } else if(opcode == XED_ICLASS_LODSB ||
	      opcode == XED_ICLASS_LODSW || opcode == XED_ICLASS_LODSD ||
	      opcode == XED_ICLASS_LODSQ) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &string_ins, IARG_END);
    } else if(opcode == XED_ICLASS_LEA) {
	INS_InsertCall(ins, IPOINT_BEFORE,  AFUNPTR(inc_cnt),
		       IARG_UINT32, &lea_ins, IARG_END);
    }
}    
#endif

void Instruction(INS ins, void* v)
{
    INT32 category;
    OPCODE opcode;

    category = INS_Category(ins);
    opcode = INS_Opcode(ins);

    cnt_ins(ins, opcode);

    if(INS_IsSyscall(ins)) {
	DPRINT(stderr, "about to instrument syscall\n");
	//instrument_syscall(ins);
    } else if(INS_IsMov(ins) || opcode == XED_ICLASS_MOVSX ||
	      opcode == XED_ICLASS_MOVZX) {
	DPRINT(stderr, "about to instrument move\n");
	instrument_mov(ins);
    } else if(category == XED_CATEGORY_CMOV) {
	DPRINT(stderr, "about to instrument cmov\n");
	instrument_cmov(ins, opcode);
    } else if(category == XED_CATEGORY_PUSH) {
	DPRINT(stderr, "about to instrument push\n");
	instrument_push(ins);
    } else if(category == XED_CATEGORY_POP) {
	DPRINT(stderr, "about to instrument pop\n");
	instrument_pop(ins);
    } else if(INS_IsXchg(ins)) {
	DPRINT(stderr, "about to instrument xchg\n");
	instrument_xchg(ins);
    } else if(opcode == XED_ICLASS_ADD || opcode == XED_ICLASS_ADC ||
	      opcode == XED_ICLASS_SUB || opcode == XED_ICLASS_SBB ||
	      opcode == XED_ICLASS_OR  || opcode == XED_ICLASS_AND ||
	      opcode == XED_ICLASS_XOR) {
	DPRINT(stderr, "about to instrument addorsub\n");
	instrument_addorsub(ins);
    } else if(opcode == XED_ICLASS_MUL || opcode == XED_ICLASS_IMUL ||
	      opcode == XED_ICLASS_DIV || opcode == XED_ICLASS_IDIV) {
	DPRINT(stderr, "about to instrument mulordvi\n");
	instrument_mulordiv(ins);
	//string instruction = INS_Disassemble(ins);
	//DPRINT(stderr, "-%s\n", instruction.c_str());
    } else if(opcode == XED_ICLASS_XADD) {
	DPRINT(stderr, "about to instruent xadd\n");
	instrument_xadd(ins);
    }else if(INS_IsRet(ins)) {
//	breakdown_ins(ins);
	DPRINT(stderr, "about to instrument return\n");
	instrument_ret(ins);
    } else if(INS_IsCall(ins) && INS_IsProcedureCall(ins)) {
	DPRINT(stderr, "about to instrument call\n");
	instrument_call(ins);
//	breakdown_ins(ins);
    } else if(opcode == XED_ICLASS_MOVSB ||
	      opcode == XED_ICLASS_MOVSW || opcode == XED_ICLASS_MOVSD ||
	      opcode == XED_ICLASS_MOVSQ) {
	DPRINT(stderr, "about to instrument move string\n");
	instrument_move_string(ins, opcode);
    } else if(opcode == XED_ICLASS_STOSB ||
	      opcode == XED_ICLASS_STOSW || opcode == XED_ICLASS_STOSD ||
	      opcode == XED_ICLASS_STOSQ) {
	DPRINT(stderr, "about to instrument store string\n");
	instrument_store_string(ins, opcode);
    } else if(opcode == XED_ICLASS_LODSB ||
	      opcode == XED_ICLASS_LODSW || opcode == XED_ICLASS_LODSD ||
	      opcode == XED_ICLASS_LODSQ) {
	DPRINT(stderr, "about to instrument load string\n");
	instrument_load_string(ins, opcode);
    } else if(opcode == XED_ICLASS_LEA) {
	cnt_ins(ins, opcode);
	instrument_lea(ins);
    } else if(opcode == XED_ICLASS_CVTSD2SS || opcode == XED_ICLASS_CVTTSD2SI) {
	DPRINT(stderr, "about top instrument CVTSD2SS\n");
	instrument_mov(ins);
    } else if(opcode == XED_ICLASS_FADD || opcode == XED_ICLASS_FADDP || opcode == XED_ICLASS_FIADD ||
	      opcode == XED_ICLASS_FSUB || opcode == XED_ICLASS_FSUBP || opcode == XED_ICLASS_FISUB ||
	      opcode == XED_ICLASS_FSUBR || opcode == XED_ICLASS_FSUBRP || opcode == XED_ICLASS_FISUBR) {
	DPRINT(stderr, "about top instrument FADD/FSUB\n");
	instrument_addorsub(ins);
    } else if(opcode == XED_ICLASS_FDIV || opcode == XED_ICLASS_FDIVP || 
	      opcode == XED_ICLASS_FIDIV || opcode == XED_ICLASS_FMUL ||
	      opcode == XED_ICLASS_FMULP || opcode == XED_ICLASS_FIMUL ||
	      opcode == XED_ICLASS_PMULLW) {
	DPRINT(stderr, "about top instrument FMUL/FDIV\n");
	instrument_mulordiv(ins);
    } else if(opcode == XED_ICLASS_FIST   || opcode == XED_ICLASS_FISTP || 
	      opcode == XED_ICLASS_FISTTP || opcode == XED_ICLASS_FST ||
	      opcode == XED_ICLASS_FSTP) {
	DPRINT(stderr, "about top instrument FIST\n");
	instrument_floatstore(ins);
    } else if(opcode == XED_ICLASS_FLD || opcode == XED_ICLASS_FILD) {
	DPRINT(stderr, "about top instrument FILD\n");
	instrument_floatload(ins);
    } else if (opcode == XED_ICLASS_FLD1 || opcode == XED_ICLASS_FLDL2T ||
	       opcode == XED_ICLASS_FLDL2E || //opcode == XED_ICLASS_FLDPI ||
	       opcode == XED_ICLASS_FLDLG2 || opcode == XED_ICLASS_FLDLN2 ||
	       opcode == XED_ICLASS_FLDZ) {
	DPRINT(stderr, "about top instrument FLDXCONSTANT\n");
	instrument_fldconstant(ins);	
    } else if (opcode == XED_ICLASS_FXCH) {
	DPRINT(stderr, "about top instrument FXCH\n");
	instrument_xchg(ins);
    } else if(opcode == XED_ICLASS_FYL2X || opcode == XED_ICLASS_FYL2XP1) {
	DPRINT(stderr, "about top instrument FYL2X\n");
	instrument_addorsub(ins);
    } else if (opcode == XED_ICLASS_PACKSSWB) {
	DPRINT(stderr, "about top instrument PACK\n");
	instrument_addorsub(ins);
    } else if(opcode == XED_ICLASS_PADDB || opcode == XED_ICLASS_PADDW ||
	      opcode == XED_ICLASS_PADDD || opcode == XED_ICLASS_PAND  || 
	      opcode == XED_ICLASS_PAVGB || opcode == XED_ICLASS_PAVGW || 
	      opcode == XED_ICLASS_PMAXUB || opcode == XED_ICLASS_PMINUB ||
	      opcode == XED_ICLASS_POR || opcode == XED_ICLASS_PXOR ||
	      opcode == XED_ICLASS_XORPS || opcode == XED_ICLASS_PSUBB ||
	      opcode == XED_ICLASS_PSUBW || opcode == XED_ICLASS_PSUBD ||
	      opcode == XED_ICLASS_PSUBQ || opcode == XED_ICLASS_XORPS) {
	DPRINT(stderr, "about top instrument PADD/MIN/XOR/SUB\n");
	instrument_addorsub(ins);
    } else if(opcode == XED_ICLASS_PCMPEQB || opcode == XED_ICLASS_PCMPEQW ||
	      opcode == XED_ICLASS_PCMPEQD || opcode == XED_ICLASS_PCMPGTB ||
	      opcode == XED_ICLASS_PCMPGTW || opcode == XED_ICLASS_PCMPGTD) {
	DPRINT(stderr, "about top instrument PCMP\n");
	instrument_pcmp(ins);
    } else if (opcode == XED_ICLASS_PUNPCKHBW || opcode == XED_ICLASS_PUNPCKHWD ||
	       opcode == XED_ICLASS_PUNPCKHDQ || opcode == XED_ICLASS_PUNPCKHQDQ ||
	       opcode == XED_ICLASS_PUNPCKLBW || opcode == XED_ICLASS_PUNPCKLWD ||
	       opcode == XED_ICLASS_PUNPCKLDQ || opcode == XED_ICLASS_PUNPCKHQDQ) {
	DPRINT(stderr, "about top instrument PUNPCK\n");
	instrument_addorsub(ins);
    }
}

void Trace(TRACE trace, void* v)
{

// Visit every basic block  in the trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
	for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)){
	    Instruction(ins, v);
 	}
    }
}

void Fini(INT32 code, void *v)
{
    fprintf(logfd, "Instrumentation completed num slow paths %u really slow %u\n", slowpath, reallyslow);
#ifdef INS_COUNT
    print_cnt();
#endif
    fprintf(stderr, "DOOT DOOT DOOT\n");
  	ofstream OutFile;
	OutFile.open(KnobOutputFile.Value().c_str());
    // Write to a file since cout and cerr maybe closed by the application
    trie_total = trie_total + taint_store.wp.get_trie_data();
    OutFile << "The number of total hits on top-level access: " << trie_total.top_hit << endl;
    OutFile << "The number of total hits on second-level access: " << trie_total.mid_hit << endl;
    OutFile << "The number of total hits on bottom-level access: " << trie_total.bot_hit << endl;
    OutFile << "The number of total changes on top-level: " << trie_total.top_change << endl;
    OutFile << "The number of total changes on second-level: " << trie_total.mid_change << endl;
    OutFile << "The number of total changes on bottom-level: " << trie_total.bot_change << endl;
    OutFile << "The number of total breaks for top-level entires: " << trie_total.top_break << endl;
    OutFile << "The number of total breaks for second-level entries: " << trie_total.mid_break << endl;
    OutFile << "Notes*: *break* means a top or second level entrie can't represent the whole page below anymore." << endl;
////////////////////////Out put the data collected

    OutFile.close();
}

void settaintvar()
{
    fprintf(logfd, "setting aftermain to 1\n");
    fflush(logfd);
    aftermain = 1;
}

string maincall("initme");

void FlagRtn(RTN rtn, void* v) 
{
    RTN_Open(rtn);
    string* name = new string(RTN_Name(rtn));
    DPRINT(errfd, "Got rtn %s\n", name->c_str());
//    fflush(errfd);
    if(*name == maincall) {
	fprintf(logfd, "found main name is %s\n", name->c_str());
	//fflush(stderr);
	RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(settaintvar),
		       IARG_END);
    }
    delete name;
    RTN_Close(rtn);
}

VOID DataInit() {
    trie_total.top_hit = 0;
    trie_total.mid_hit = 0;
    trie_total.bot_hit = 0;
    trie_total.top_change = 0;
    trie_total.mid_change = 0;
    trie_total.bot_change = 0;
    trie_total.top_break = 0;
    trie_total.mid_break = 0;
}

int main(int argc, char* argv[])
{
    int i;
    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    PIN_Init(argc,argv);

    DataInit();

    logfd = fopen("/tmp/taint.out", "w");
    if(logfd == NULL) {
	fprintf(stderr, "failed to open logfile\n");
	return 0;
    }

    fprintf(logfd, "sizeof pages %d grbase is %d stack ptr reg is %d\n", sizeof(pages),  REG_GR_BASE, REG_ESP);
    //initialize page_table version of taint tracking
    //    memset(shady_fd, 0, sizeof(shady_fd));
    memset(ZERO_PAGE, 0, sizeof(ZERO_PAGE));
    //    memset(pages, 0, sizeof(pages));
    for(i = 0; i < MAX_PAGES; i++) {
      pages[i] = ZERO_PAGE_LOC;
    }
    memset(global_regs, 0, REGS_AVAIL);
    fprintf(logfd, "zero page addr is 0x%x loc is 0x%x size is %d\n", (u_int) ZERO_PAGE, (u_int) ZERO_PAGE_LOC, sizeof(ZERO_PAGE));
    //initialize mem dep tracking

    errfd = fopen("/tmp/taint.err", "w");
    if(errfd == NULL) {
	fprintf(stderr, "failed to open error log file\n");
	return 0;
    }
    snpfd = fopen("/tmp/taint.snapshots", "w");
    if(snpfd == NULL) {
	fprintf(stderr, "failed to open snapshot log\n");
	return 0;
    }
    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);   
    RTN_AddInstrumentFunction(FlagRtn, 0);
    PIN_AddSyscallEntryFunction(instrument_syscall_entry, 0);
    PIN_AddSyscallExitFunction(instrument_syscall_exit, 0);
    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);
//    TRACE_AddInstrumentFunction(Trace, 0);
    //Start program
    PIN_StartProgram();
}
