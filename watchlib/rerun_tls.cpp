/** Copyright 2010 University of Michigan

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. **/
#ifndef vector_H
#include <deque>
#define vector_H
#endif

#include <map>
#include <iostream>
#include <fstream>
#include "pin.H"
#include "auto_wp.h"

#define RANGE_CACHE
#define PAGE_TABLE

// Force each thread's data to be in its own data cache line so that
// multiple threads do not contend for the same data cache line.
#define PADSIZE 56  // 64 byte line size: 64-8
#define MEM_SIZE    0//(-1 & ~4194303)//0   // 0xffffffff as the max vertual memory address.

using std::deque;
using Hongyi_WatchPoint::WatchPoint;
using Hongyi_WatchPoint::trie_data_t;
#ifdef RANGE_CACHE
using Hongyi_WatchPoint::range_data_t;
#endif
#ifdef PAGE_TABLE
using Hongyi_WatchPoint::pagetable_data_t;
#endif
using Hongyi_WatchPoint::MEM_WatchPoint;
//My own data
struct thread_wp_data_t
{
    MEM_WatchPoint<ADDRINT, UINT32> mem;
    WatchPoint<ADDRINT, UINT32> wp;
    UINT64 number_of_instructions;
    UINT8 pad[PADSIZE];
};

map<THREADID,thread_wp_data_t*> thread_map;
map<THREADID,thread_wp_data_t*>::iterator thread_map_iter;

deque<THREADID> live_threads;

UINT64 instruction_total;
trie_data_t trie_total;
deque<trie_data_t> total_trie_data;
#ifdef RANGE_CACHE
range_data_t range_total;
deque<unsigned long long> total_max_range_num;
deque<unsigned long long> total_avg_range_num;
deque<range_data_t> total_range_data;
#endif
#ifdef PAGE_TABLE
pagetable_data_t pagetable_total;
deque<pagetable_data_t> total_pagetable_data;
#endif

//My own data
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "rerun_tls.out", "specify output file name");

PIN_LOCK init_lock;

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    GetLock(&init_lock, threadid+1);//get LOCK
    thread_wp_data_t* this_thread = new thread_wp_data_t;

    this_thread->number_of_instructions = 0;

    (this_thread->wp).add_watch_wp(0, MEM_SIZE);
    
    thread_map[threadid] = this_thread;
    live_threads.push_back(threadid);
    ReleaseLock(&init_lock);//release lOCK
}

VOID AddThreadData(thread_wp_data_t* thread_to_add)
{
    instruction_total += thread_to_add->number_of_instructions;
    trie_total = trie_total + (thread_to_add->wp).get_trie_data();
    total_trie_data.push_back( (thread_to_add->wp).get_trie_data() );
#ifdef RANGE_CACHE
    range_total = range_total + (thread_to_add->wp).get_range_data();
    total_max_range_num.push_back( ( (thread_to_add->wp).get_range_data() ).max_range_num );
    total_avg_range_num.push_back( ((thread_to_add->wp).get_range_data()).total_cur_range_num / ((thread_to_add->wp).get_range_data()).changes );
    total_range_data.push_back( (thread_to_add->wp).get_range_data() );
#endif
#ifdef PAGE_TABLE
    pagetable_total = pagetable_total + (thread_to_add->wp).get_pagetable_data();
    total_pagetable_data.push_back( (thread_to_add->wp).get_pagetable_data() );
#endif
}

VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    deque<THREADID>::iterator iter;
    GetLock(&init_lock, threadid+1);//get LOCK
    AddThreadData(thread_map[threadid]);
    delete thread_map[threadid];
    thread_map.erase (threadid);
    for(iter = live_threads.begin(); iter != live_threads.end(); iter++) {
        if(*iter == threadid) {
            live_threads.erase(iter);
            break;
        }
    }
    ReleaseLock(&init_lock);//release LOCK
}

// Prepare a memory read
VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)
{
    thread_wp_data_t* this_thread = thread_map[threadid];
    if ( (this_thread->wp).read_fault( (ADDRINT) (addr), (ADDRINT) (size) ) ) {
        thread_wp_data_t* object_thread;
        GetLock(&init_lock, threadid+1);//get LOCK
        for (thread_map_iter = thread_map.begin(); thread_map_iter != thread_map.end(); thread_map_iter++) {
            if (thread_map_iter->first != threadid) {
                object_thread = thread_map_iter->second;
                if ( (object_thread->mem).write_fault((ADDRINT) (addr), (ADDRINT) (size) ) ) {
                    (this_thread->mem).clear();
                    (this_thread->wp).add_watch_wp(0, MEM_SIZE);
                    ReleaseLock(&init_lock);//release LOCK
                    return;
                }
            }
        }
        
        (this_thread->wp).rm_read ((ADDRINT) (addr), (ADDRINT) (size) );
        (this_thread->mem).add_read_wp((ADDRINT) (addr), (ADDRINT) (size) );
        ReleaseLock(&init_lock);//release LOCK
    }
    return;
}

// Prepare a memory write
VOID RecordMemWrite(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)//, THREADID threadid)
{
    thread_wp_data_t* this_thread = thread_map[threadid];

    if ( (this_thread->wp).write_fault((ADDRINT) (addr), (ADDRINT) (size) ) ) {
        thread_wp_data_t* object_thread;
        GetLock(&init_lock, threadid+1);//get LOCK
        for (thread_map_iter = thread_map.begin(); thread_map_iter != thread_map.end(); thread_map_iter++) {
            if (thread_map_iter->first != threadid) {
                object_thread = thread_map_iter->second;
                if ( (object_thread->mem).watch_fault((ADDRINT) (addr), (ADDRINT) (size) ) ) {
                    (this_thread->mem).clear();
                    (this_thread->wp).add_watch_wp(0, MEM_SIZE);
                    ReleaseLock(&init_lock);//release LOCK
                    return;
                }
            }
        }
        
        (this_thread->wp).rm_write ((ADDRINT) (addr), (ADDRINT) (size) );
        (this_thread->mem).add_write_wp((ADDRINT) (addr), (ADDRINT) (size) );
        ReleaseLock(&init_lock);//release LOCK
    }
    return;
}

// This function is called before every block
VOID PIN_FAST_ANALYSIS_CALL docount(ADDRINT c, THREADID tid)
{
    thread_map[tid]->number_of_instructions += c;
}

// Count the number of each specific instruction.
VOID Trace(TRACE trace, VOID *v)
{
    // Visit every basic block  in the trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        // Insert a call to docount for every bbl, passing the number of instructions.
        // IPOINT_ANYWHERE allows Pin to schedule the call anywhere in the bbl to obtain best performance.
        BBL_InsertCall(bbl, IPOINT_ANYWHERE, (AFUNPTR)docount, IARG_FAST_ANALYSIS_CALL, IARG_UINT32, 
                BBL_NumIns(bbl), IARG_THREAD_ID, IARG_END);
    }
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
            INS_InsertCall  (
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
    deque<THREADID>::iterator live_iter;
    for(live_iter = live_threads.begin(); live_iter != live_threads.end(); live_iter++) {
        AddThreadData(thread_map[*live_iter]);
    }
    ofstream OutFile;
    OutFile.open(KnobOutputFile.Value().c_str());
    // Write to a file since cout and cerr maybe closed by the application
    OutFile << "Total number of instructions: " << instruction_total << endl;
    OutFile << endl << "**Trie data: \n" << endl;
    OutFile << "The number of total top-level accesses: " << trie_total.top_hit << endl;
    OutFile << "The number of total mid-level accesses: " << trie_total.mid_hit << endl;
    OutFile << "The number of total bot-level accesses: " << trie_total.bot_hit << endl;
    OutFile << "The number of total top-level changes: " << trie_total.top_change << endl;
    OutFile << "The number of total mid-level changes: " << trie_total.mid_change << endl;
    OutFile << "The number of total bot-level changes: " << trie_total.bot_change << endl;
    OutFile << "The number of total top-level breaks: " << trie_total.top_break << endl;
    OutFile << "The number of total mid-level breaks: " << trie_total.mid_break << endl;
    OutFile << "Notes*: *break* means a top or second level entry can't represent the whole page below anymore." << endl << endl;

    OutFile << "The number of total top-level WLB hits: " << trie_total.wlb_hit_top << endl;
    OutFile << "The number of total mid-level WLB hits: " << trie_total.wlb_hit_mid << endl;
    OutFile << "The number of total bot-level WLB hits: " << trie_total.wlb_hit_bot << endl;
    OutFile << "The number of total top-level WLB misses: " << trie_total.wlb_miss_top << endl;
    OutFile << "The number of total mid-level WLB misses: " << trie_total.wlb_miss_mid << endl;
    OutFile << "The number of total bot-level WLB misses: " << trie_total.wlb_miss_bot << endl << endl;
    
    deque<trie_data_t>::iterator trie_iter;
    for (trie_iter = total_trie_data.begin(); trie_iter != total_trie_data.end(); trie_iter++) {
        OutFile << "The number of top-level accesses for this thread: " << trie_iter->top_hit << endl;
        OutFile << "The number of mid-level accesses for this thread: " << trie_iter->mid_hit << endl;
        OutFile << "The number of bot-level accesses for this thread: " << trie_iter->bot_hit << endl;
        OutFile << "The number of top-level changes for this thread: " << trie_iter->top_change << endl;
        OutFile << "The number of mid-level changes for this thread: " << trie_iter->mid_change << endl;
        OutFile << "The number of bot-level changes for this thread: " << trie_iter->bot_change << endl;
        OutFile << "The number of top-level breaks for this thread: " << trie_iter->top_break << endl;
        OutFile << "The number of mid-level breaks for this thread: " << trie_iter->mid_break << endl;
        OutFile << "The number of top-level WLB hits for this thread: " << trie_iter->wlb_hit_top << endl;
        OutFile << "The number of mid-level WLB hits for this thread: " << trie_iter->wlb_hit_mid << endl;
        OutFile << "The number of bot-level WLB hits for this thread: " << trie_iter->wlb_hit_bot << endl;
        OutFile << "The number of top-level WLB miss for this thread: " << trie_iter->wlb_miss_top << endl;
        OutFile << "The number of mid-level WLB miss for this thread: " << trie_iter->wlb_miss_mid << endl;
        OutFile << "The number of bot-level WLB miss for this thread: " << trie_iter->wlb_miss_bot << endl;
        OutFile << "----" << endl;
    }
#ifdef RANGE_CACHE
    OutFile << endl << "**Range_cache data: " << endl;
    OutFile << "The average number of ranges in the system: " << ((double)range_total.total_cur_range_num/range_total.changes) << endl;
    OutFile << "The number of range hits: " << range_total.hit << endl;
    OutFile << "The number of range misses: " << range_total.miss << endl;
    OutFile << "The number of ranges kicked out: " << range_total.kick << endl;
    OutFile << "The number of times dirty ranges were kicked: " << range_total.dirty_kick << endl << endl;
    
    deque<unsigned long long>::iterator iter;
    deque<range_data_t>::iterator range_iter;
    for (iter = total_max_range_num.begin(); iter != total_max_range_num.end(); iter++) {
        OutFile << "The max_range_num for this thread is: " << *iter << endl;
    }

    OutFile << endl;

    for (range_iter = total_range_data.begin(); range_iter != total_range_data.end(); range_iter++) {
        OutFile << "The avg_range_num for this thread is: " << (range_iter->total_cur_range_num/range_iter->changes) << endl;
        OutFile << "The number of hits in this thread: " << range_iter->hit << endl;
        OutFile << "The number of miss in this thread: " << range_iter->miss << endl;
        OutFile << "The number of ranges kicked from this thread: " << range_iter->kick << endl;
        OutFile << "The number of times dirty ranges were kicked from this thread " << range_iter->dirty_kick << endl;
        OutFile << "----" << endl;
    }

#endif
#ifdef PAGE_TABLE
    deque<pagetable_data_t>::iterator pagetable_iter;
    OutFile << endl << "**PageTable data: " << endl;
    OutFile << "The number of accesses to a page marked as watched: " << pagetable_total.access << endl;
    OutFile << "The number of accesses to a real watchpoint: " << pagetable_total.wp_hit << endl;
    
    OutFile << endl;

    for (pagetable_iter = total_pagetable_data.begin(); pagetable_iter != total_pagetable_data.end(); pagetable_iter++) {
        OutFile << "The number of accesses to pages marked in this thread: " << pagetable_iter->access << endl;
        OutFile << "The number of accesses to real watchpoint in this thread: " << pagetable_iter->wp_hit << endl;
        OutFile << "----" << endl;
    }
#endif
////////////////////////Out put the data collected

    OutFile.close();
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
    return;
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "Rerun Watchpoint system." << endl;
    cerr << "  Just give this a parallel program to run." << endl;
    cerr << "Will give output data in rerun_tls.out unless you give ";
    cerr << "it a -o {name} option." << endl;
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
    
    DataInit();

    // Register ThreadStart to be called when a thread starts.
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    TRACE_AddInstrumentFunction(Trace, 0);

    // Register Instruction to be called to instrument instructions.
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits.
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
