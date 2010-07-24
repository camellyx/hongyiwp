/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2010 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
#ifndef vector_H
#include <deque>
#define vector_H
#endif

#include <map>
#include <iostream>
#include <fstream>
#include "pin.H"
#include "auto_wp.h"

//#define RANGE_CACHE

using namespace std;
using namespace Hongyi_WatchPoint;
//My own data

deque<unsigned long long> total_max_range_num;

struct	thread_mem_data_t {//This data will not vanish as the thread exit. It would be delete only when parent thread has quited.
	MEM_WatchPoint<ADDRINT, UINT32> mem;
	trie_data_t		trie;
#ifdef RANGE_CACHE
	range_data_t	range;
#endif
};

struct thread_wp_data_t
{
	//As a parent:
	INT32						child_thread_num;//check if all child thread has finished.
	deque<thread_mem_data_t*>	child_data;
	//As a child:
	OS_THREAD_ID				parent_threadid;//this points to its parent thread
	thread_mem_data_t*			self_mem_ptr;//points to its owndata, which is stored by its parent.
	//As a thread itself:
	bool						root;//this tells whether if the the thread is root. Well if it's the root thread then parent_thread_id would be invalid.
	WatchPoint<ADDRINT, UINT32>	wp;
};

INT32 thread_num = 0;//Well this is the only thing to determine whether if a thread is a root or not. If thread_num = 0. Then the thread is a root.
thread_mem_data_t	root_mem_data;//This is where root store its data(root has no parents)

map<OS_THREAD_ID,thread_wp_data_t*>				thread_map;
map<OS_THREAD_ID,thread_wp_data_t*>::iterator	thread_map_iter;

//My own data
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "grace_tls.out", "specify output file name");

PIN_LOCK init_lock;

// Force each thread's data to be in its own data cache line so that
// multiple threads do not contend for the same data cache line.
// This avoids the false sharing problem.
#define PADSIZE 56  // 64 byte line size: 64-8
#define MEM_SIZE	0	// 0xffffffff as the max vertual memory address.
				
bool thread_commit_data_conflict(MEM_WatchPoint<ADDRINT, UINT32>& sibling_mem, MEM_WatchPoint<ADDRINT, UINT32>& this_mem) {
	watchpoint_t<ADDRINT, UINT32> temp;//temp will hold the watchpoint_t struct dumped out from object_thread
	(this_mem).DumpStart();//It would get read to dump all the "read/write set" out;
	while (!(this_mem).DumpEnd() ) {
		temp = (this_mem).Dump();//This would dump out all the wp one by one.
		if (temp.flags & WA_WRITE) {//If this is a write, then check both read and write.
			if (sibling_mem.watch_fault(temp.addr, temp.size) )
				return true;
		}
		if (temp.flags & WA_READ) {//If this is a read, then check write.
			if (sibling_mem.write_fault(temp.addr, temp.size) )
				return true;
		}
	}
	return false;
}	

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
	GetLock(&init_lock, threadid+1);//get LOCK
	OS_THREAD_ID		this_threadid = PIN_GetTid();
	thread_wp_data_t*	this_thread = new thread_wp_data_t;
	if (thread_num == 0) {
		this_thread->root = true;
		this_thread->self_mem_ptr = &root_mem_data;//root has no parents so it stores the data at root_mem_data
	}
	else {
		thread_wp_data_t*	parent_thread;
		this_thread->self_mem_ptr = new thread_mem_data_t;//creat a new mem for itself
		this_thread->root = false;
		this_thread->parent_threadid = PIN_GetParentTid();//get the pointer points to its parent thread
		parent_thread = thread_map[this_thread->parent_threadid];
		parent_thread->child_data.push_back(this_thread->self_mem_ptr);//insert the mem for this thread into its parent's data. In the order of thread create time
		parent_thread->child_thread_num++;//parent child_thread_num++
	}
	this_thread->child_thread_num = 0;
	(this_thread->wp).add_watch_wp(0, MEM_SIZE);
	
	thread_map[this_threadid] = this_thread;
	thread_num++;
		
	ReleaseLock(&init_lock);//release lOCK
}

VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
	OS_THREAD_ID this_threadid = PIN_GetTid();
	thread_wp_data_t* this_thread = thread_map[this_threadid];
    thread_mem_data_t* child_mem_ptr;
    thread_mem_data_t* parent_mem_ptr;
    thread_mem_data_t* compare_mem_ptr;
    trie_data_t temp_trie;
#ifdef RANGE_CACHE
    range_data_t temp_range;
#endif
	while (1) {
		GetLock(&init_lock, threadid+1);//get LOCK
		if (this_thread->child_thread_num == 0)//Only when it becomes a leaf and has no child threads, can this thread ends. 
			break;
		ReleaseLock(&init_lock);//release lOCK
        sleep(1);
	}

    temp_trie = this_thread->wp.get_trie_data();
#ifdef RANGE_CACHE
    temp_range = this_thread->wp.get_range_data();
    this_thread->self_mem_ptr->trie = this_thread->self_mem_ptr->trie + temp_trie;
    this_thread->self_mem_ptr->range = this_thread->self_mem_ptr->range + temp_range;
#endif

    if (!this_thread->root) {
        thread_map[this_thread->parent_threadid]->child_thread_num--;

        if (thread_map[this_thread->parent_threadid]->child_thread_num == 0) {
            deque<thread_mem_data_t*>::iterator child_iter;
            deque<thread_mem_data_t*>::iterator compare_iter;

            parent_mem_ptr = thread_map[this_thread->parent_threadid]->self_mem_ptr;

            for (child_iter = (thread_map[this_thread->parent_threadid]->child_data).end() -1;
                    child_iter != (thread_map[this_thread->parent_threadid]->child_data).begin();
                    child_iter--) {
                child_mem_ptr = *child_iter;
                parent_mem_ptr->trie = parent_mem_ptr->trie + child_mem_ptr->trie;
#ifdef RANGE_CACHE
                parent_mem_ptr->range = parent_mem_ptr->range + child_mem_ptr->range;
#endif
                for (compare_iter = child_iter - 1;
                        compare_iter != (thread_map[this_thread->parent_threadid]->child_data).begin();
                        compare_iter--) {
                    compare_mem_ptr = *compare_iter;
                    if (thread_commit_data_conflict(compare_mem_ptr->mem, child_mem_ptr->mem) ) {
                        parent_mem_ptr->trie = parent_mem_ptr->trie + child_mem_ptr->trie;
#ifdef RANGE_CACHE
                        parent_mem_ptr->range = parent_mem_ptr->range + child_mem_ptr->range;
#endif
                        break;
                    }
                }
                delete child_mem_ptr;
            }
            child_mem_ptr = *child_iter; // begin() base case
            parent_mem_ptr->trie = parent_mem_ptr->trie + child_mem_ptr->trie;
#ifdef RANGE_CACHE
            parent_mem_ptr->range = parent_mem_ptr->range + parent_mem_ptr->range;
            total_max_range_num.push_back( (child_mem_ptr->range).max_range_num);
#endif
            delete child_mem_ptr;
            (thread_map[this_thread->parent_threadid]->child_data).clear();
        }

        delete thread_map[this_threadid];
        thread_map.erase (this_threadid);
        thread_num--;
    }
#ifdef RANGE_CACHE
    else {
        total_max_range_num.push_back( (this_thread->self_mem_ptr->range).max_range_num);
    }
#endif
    ReleaseLock(&init_lock);//release LOCK
}

// This would check for read watchfault. And save it to mem(as read set) if there are any.
VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)
{
	GetLock(&init_lock, threadid+1);//get LOCK
	OS_THREAD_ID this_threadid = PIN_GetTid();
	thread_wp_data_t* this_thread = thread_map[this_threadid];
    if ( (this_thread->wp).read_fault( (ADDRINT) (addr), (ADDRINT) (size) ) ) {
		(this_thread->wp).rm_read ((ADDRINT) (addr), (ADDRINT) (size) );
		(this_thread->self_mem_ptr->mem).add_read_wp((ADDRINT) (addr), (ADDRINT) (size) );
	}
	ReleaseLock(&init_lock);//release LOCK
	return;
}

// This would check for write watchfault. And save it to mem(as read set) if there are any.
VOID RecordMemWrite(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)//, THREADID threadid)
{
	GetLock(&init_lock, threadid+1);//get LOCK
	OS_THREAD_ID this_threadid = PIN_GetTid();
	thread_wp_data_t* this_thread = thread_map[this_threadid];
    if ( (this_thread->wp).write_fault((ADDRINT) (addr), (ADDRINT) (size) ) ) {
		(this_thread->wp).rm_write ((ADDRINT) (addr), (ADDRINT) (size) );
		(this_thread->self_mem_ptr->mem).add_write_wp((ADDRINT) (addr), (ADDRINT) (size) );
	}
	ReleaseLock(&init_lock);//release LOCK
	return;
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // The IA-64 architecture has explicitly predicated instructions. 
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_MEMORYREAD_SIZE,
				IARG_THREAD_ID,
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertCall	(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_MEMORYWRITE_SIZE,
				IARG_THREAD_ID,
                IARG_END);
        }
    }
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    // Write to a file since cout and cerr maybe closed by the application
    ofstream OutFile;
    OutFile.open(KnobOutputFile.Value().c_str());
    OutFile << "**Trie data: \n" << endl;
    OutFile << "The number of total hits on top-level access: " << root_mem_data.trie.top_hit << endl;
    OutFile << "The number of total hits on second-level access: " << root_mem_data.trie.mid_hit << endl;
    OutFile << "The number of total hits on bottom-level access: " << root_mem_data.trie.bot_hit << endl;
    OutFile << "The number of total changes on top-level: " << root_mem_data.trie.top_change << endl;
    OutFile << "The number of total changes on second-level: " << root_mem_data.trie.mid_change << endl;
    OutFile << "The number of total changes on bottom-level: " << root_mem_data.trie.bot_change << endl;
    OutFile << "The number of total breaks for top-level entires: " << root_mem_data.trie.top_break << endl;
    OutFile << "The number of total breaks for second-level entries: " << root_mem_data.trie.mid_break << endl;
    OutFile << "Notes*: *break* means a top or second level entrie can't represent the whole page below anymore." << endl << endl;

    OutFile << "The number of total WLB top-level hits: " << root_mem_data.trie.wlb_hit_top << endl;
    OutFile << "The number of total WLB mid-level hits: " << root_mem_data.trie.wlb_hit_mid << endl;
    OutFile << "The number of total WLB bot-level hits: " << root_mem_data.trie.wlb_hit_bot << endl;
    OutFile << "The number of total WLB top-level misses: " << root_mem_data.trie.wlb_miss_top << endl;
    OutFile << "The number of total WLB mid-level misses: " << root_mem_data.trie.wlb_miss_mid << endl;
    OutFile << "The number of total WLB bot-level misses: " << root_mem_data.trie.wlb_miss_bot << endl << endl;
   
#ifdef RANGE_CACHE 
    OutFile << "**Range_cache data: \n" << endl;
    OutFile << "The number of average ranges in the system: " << root_mem_data.range.avg_range_num << endl;
    OutFile << "The number of hits in the system: " << root_mem_data.range.hit << endl;
    OutFile << "The number of miss in the system: " << root_mem_data.range.miss << endl;
    OutFile << "The number of range kickouts in the system: " << root_mem_data.range.kick << endl << endl;
    OutFile << "Below are max_range_num for each thread: " << endl;
    
    deque<unsigned long long>::iterator iter;
    for (iter = total_max_range_num.begin(); iter != total_max_range_num.end(); iter++) {
    	OutFile << "The max_range_num for this thread is: " << *iter << endl;
    }
#endif
////////////////////////Out put the data collected
    OutFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // Initialize pin
    PIN_InitSymbols();
    if (PIN_Init(argc, argv)) return Usage();

    // Initialize the init_lock
    InitLock(&init_lock);

    // Register ThreadStart to be called when a thread starts.
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    // Register Instruction to be called to instrument instructions.
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits.
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
