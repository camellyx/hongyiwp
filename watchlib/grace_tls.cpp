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

#define MEM_SIZE 0 // Must be this way currently.

using namespace std;
using namespace Hongyi_WatchPoint;
//My own data

deque<unsigned long long> total_max_range_num;

struct  thread_mem_data_t {
    //This data will not vanish as the thread exit. It would be delete only after parent thread has stopped.
    MEM_WatchPoint<ADDRINT, UINT32> mem;
    UINT64          total_instructions;
    trie_data_t     trie;
#ifdef RANGE_CACHE
    range_data_t    range;
#endif
#ifdef PAGE_TABLE
    pagetable_data_t pagetable;
#endif
};

deque<trie_data_t> total_trie_data;
#ifdef RANGE_CACHE
deque<range_data_t> total_range_data;
#endif
#ifdef PAGE_TABLE
deque<pagetable_data_t> total_pagetable_data;
#endif

struct thread_wp_data_t
{
    //As a parent:
    INT32                       child_thread_num; //check if all child thread has finished.
    deque<thread_mem_data_t*>   child_data;
    deque<OS_THREAD_ID>         children_thread_ids;
    //As a child:
    OS_THREAD_ID                parent_threadid; //this points to its parent thread
    thread_mem_data_t*          self_mem_ptr; //points to its own data, which is stored by its parent.
    //As a thread itself:
    bool                        root; //this tells whether if the the thread is root. Well if it's the root thread then parent_thread_id would be invalid.
    bool                        has_had_children;
    bool                        has_had_siblings;
    bool                        children_skipped_kill;
    bool                        sibling_skipped_kill;
    WatchPoint<ADDRINT, UINT32> wp;
};

INT32 thread_num = 0;//Well this is the only thing to determine whether if a thread is a root or not. If thread_num = 0. Then the thread is a root.
thread_mem_data_t   root_mem_data;//This is where root store its data(root has no parents)

map<OS_THREAD_ID,thread_wp_data_t*>             thread_map;
map<OS_THREAD_ID,thread_wp_data_t*>::iterator   thread_map_iter;

//My own data
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "grace_tls.out", "specify output file name");

PIN_LOCK init_lock;

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

VOID restart_thread_counts(thread_wp_data_t* this_thread)
{
    thread_wp_data_t* parent_thread = thread_map[this_thread->parent_threadid];
    deque<thread_mem_data_t*>::iterator child_iter;
    if (!this_thread->root) {
        for (child_iter = parent_thread->child_data.begin();
                child_iter != parent_thread->child_data.end();
                child_iter++) {
            if ((*child_iter) == this_thread->self_mem_ptr) {
                parent_thread->child_data.erase(child_iter);
                break;
            }
        }
        delete this_thread->self_mem_ptr;
        this_thread->self_mem_ptr = new thread_mem_data_t;
        parent_thread->child_data.push_back(this_thread->self_mem_ptr);
    }
    else {
        delete this_thread->self_mem_ptr;
        this_thread->self_mem_ptr = new thread_mem_data_t;
    }
}

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    GetLock(&init_lock, threadid+1);
    OS_THREAD_ID        this_threadid = PIN_GetTid();
    thread_wp_data_t*   this_thread = new thread_wp_data_t;
    this_thread->children_skipped_kill = false;
    this_thread->sibling_skipped_kill = false;
    this_thread->has_had_siblings = false;
    this_thread->has_had_children = false;

    this_thread->self_mem_ptr = new thread_mem_data_t;
    this_thread->self_mem_ptr->total_instructions = 0;
    if (thread_num == 0) {
        // Root, so it can't have a parent.  Because it doesn't have a parent,
        // we skip most of the setup code.
        this_thread->root = true;
    }
    else {
        thread_wp_data_t*   parent_thread;
        this_thread->root = false;
        // Get the pointer to its parent thread.
        this_thread->parent_threadid = PIN_GetParentTid();
        parent_thread = thread_map[this_thread->parent_threadid];
        // Insert the memory for this thread into its parent's data in
        // the order of thread creation.
        parent_thread->child_data.push_back(this_thread->self_mem_ptr);
        // Set the parent to know that it has ever had a child.
        parent_thread->has_had_children = true;
        parent_thread->child_thread_num++;

        // Set all the siblings of this thread to know that they have a sibling.
        deque<OS_THREAD_ID>::iterator sibling_iter;
        if (parent_thread->children_thread_ids.begin() != parent_thread->children_thread_ids.end())
            this_thread->has_had_siblings = true;
        for(sibling_iter = parent_thread->children_thread_ids.begin();
                sibling_iter != parent_thread->children_thread_ids.end();
                sibling_iter++) {
            thread_map[*sibling_iter]->has_had_siblings = true;
        }
        parent_thread->children_thread_ids.push_back(this_threadid);
    }
    this_thread->child_thread_num = 0;
    (this_thread->wp).add_watch_wp(0, MEM_SIZE);
    
    thread_map[this_threadid] = this_thread;
    thread_num++;

    ReleaseLock(&init_lock);
}

// This is used when a conflict happens and we need to add the stuff into
// the total a second time. 
VOID add_data_to_total(thread_mem_data_t* add_mem_ptr)
{
    root_mem_data.total_instructions += add_mem_ptr->total_instructions;
    root_mem_data.trie = root_mem_data.trie + add_mem_ptr->trie;
#ifdef RANGE_CACHE
    root_mem_data.range = root_mem_data.range + add_mem_ptr->range;
#endif
#ifdef PAGE_TABLE
    root_mem_data.pagetable = root_mem_data.pagetable + add_mem_ptr->pagetable;
#endif
}

// This doubles the values contained in a thread due to a conflict happening
VOID double_data(thread_mem_data_t* double_mem_ptr)
{
    double_mem_ptr->trie = double_mem_ptr->trie + double_mem_ptr->trie;
#ifdef RANGE_CACHE
    double_mem_ptr->range = double_mem_ptr->range + double_mem_ptr->range;
#endif
#ifdef PAGE_TABLE
    double_mem_ptr->pagetable = double_mem_ptr->pagetable + double_mem_ptr->pagetable;
#endif
}

// This keeps track of each individual thread's contribution.
VOID push_data_to_total_stacks(thread_mem_data_t* push_mem_ptr)
{
    total_trie_data.push_back( push_mem_ptr->trie );
#ifdef RANGE_CACHE
    total_range_data.push_back( push_mem_ptr->range );
    total_max_range_num.push_back( (push_mem_ptr->range).max_range_num );
#endif
#ifdef PAGE_TABLE
    total_pagetable_data.push_back( push_mem_ptr->pagetable );
#endif
}

VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    OS_THREAD_ID this_threadid;
    thread_wp_data_t* this_thread;
    OS_THREAD_ID this_parent_threadid;
    thread_mem_data_t* child_mem_ptr;
    thread_mem_data_t* parent_mem_ptr;
    thread_mem_data_t* compare_mem_ptr;
    trie_data_t temp_trie;
#ifdef RANGE_CACHE
    range_data_t temp_range;
#endif
#ifdef PAGE_TABLE
    pagetable_data_t temp_pagetable;
#endif
    GetLock(&init_lock, threadid+1);

    this_threadid = PIN_GetTid();
    this_thread = thread_map[this_threadid];
    this_parent_threadid = this_thread->parent_threadid;

    temp_trie = this_thread->wp.get_trie_data();
    this_thread->self_mem_ptr->trie = this_thread->self_mem_ptr->trie + temp_trie;
#ifdef RANGE_CACHE
    temp_range = this_thread->wp.get_range_data();
    this_thread->self_mem_ptr->range = this_thread->self_mem_ptr->range + temp_range;
#endif
#ifdef PAGE_TABLE
    temp_pagetable = this_thread->wp.get_pagetable_data();
    this_thread->self_mem_ptr->pagetable = this_thread->self_mem_ptr->pagetable + temp_pagetable;
#endif

    if (!this_thread->root) { // Never kill off the root thread
        // This thread is done, so remove one of the parente thread's children.
        thread_map[this_parent_threadid]->child_thread_num--;

        if (thread_map[this_parent_threadid]->child_thread_num == 0) {
            // If all the CURRENT children for the parent thread are now done, we need to run grace comparison against them.
            deque<thread_mem_data_t*>::iterator child_iter;
            deque<thread_mem_data_t*>::iterator compare_iter;

            parent_mem_ptr = thread_map[this_parent_threadid]->self_mem_ptr;

            for (child_iter = (thread_map[this_parent_threadid]->child_data).end() -1;
                    child_iter != (thread_map[this_parent_threadid]->child_data).begin();
                    child_iter--) {
                bool did_conflict = false;
                child_mem_ptr = *child_iter;
                // The total number of trie/range misses sets etc the system
                // sees is at least how many happened in each child thread.
                add_data_to_total(child_mem_ptr);
                for (compare_iter = (thread_map[this_parent_threadid]->child_data).begin();
                        compare_iter != child_iter;
                        compare_iter++) {
                    compare_mem_ptr = *compare_iter;
                    if (thread_commit_data_conflict(compare_mem_ptr->mem, child_mem_ptr->mem) ) {
                        // There was a conflict between a thread and an earlier sibling thread.
                        // Therefore, the second thread had to run twice.
                        did_conflict = true;
                        add_data_to_total(child_mem_ptr);
                        double_data(child_mem_ptr);
                        break;
                    }
                }
                if (!did_conflict && thread_commit_data_conflict(parent_mem_ptr->mem, child_mem_ptr->mem) ) {
                    // Didn't conflict with any other child thread, but conflicted with the parent.
                    add_data_to_total(child_mem_ptr);
                    double_data(child_mem_ptr);
                }
                // Always push this thread's total data to thread-collection.
                push_data_to_total_stacks(child_mem_ptr);
            }
            child_mem_ptr = *child_iter; // Must also check thread at begin()
            add_data_to_total(child_mem_ptr);
            if (thread_commit_data_conflict(parent_mem_ptr->mem, child_mem_ptr->mem) ) {
                add_data_to_total(child_mem_ptr);
                double_data(child_mem_ptr);
            }
            push_data_to_total_stacks(child_mem_ptr);
            // The parent will either be handled when IT dies, or will be handled by
            // the else staement below if it's the root.
        }
    }
    else {
        // Can't have conflict if this is a root thread.
        add_data_to_total(this_thread->self_mem_ptr);
        push_data_to_total_stacks(this_thread->self_mem_ptr);
    }

    // Delete stuff now.
    if (!this_thread->root) {
        if (thread_map[this_parent_threadid]->child_thread_num == 0) {
            // Thread information cleanup.
            if (!this_thread->has_had_siblings && !this_thread->has_had_children) {
                // If this guy had no siblings or children, no one will clean him up.
                // He must do it himself.
                delete this_thread->self_mem_ptr;
                delete thread_map[this_threadid];
                thread_map.erase(this_threadid);
                thread_num--;
            }
            else {
                // This thread has either siblings or children.  We're in here because the parent
                // has no more life children, so we should first try to delete all siblings.
                deque<OS_THREAD_ID>::iterator sibling_thread_id;
                // Walking through the siblings will also catch us.
                for(sibling_thread_id = (thread_map[this_parent_threadid]->children_thread_ids).begin();
                        sibling_thread_id != (thread_map[this_parent_threadid]->children_thread_ids).end();
                        sibling_thread_id++) {
                    thread_wp_data_t *sibling_thread = thread_map[*sibling_thread_id];
                    if (!thread_map[this_parent_threadid]->child_thread_num) {
                        // If the sibling never had children it is absolutely OK to delete its data.
                        // If the sibling HAD children but the number is at zero, then there is
                        // no one left to delete it, so we must.
                        if (!sibling_thread->has_had_children || !sibling_thread->child_thread_num){
                            delete sibling_thread->self_mem_ptr;
                            delete thread_map[*sibling_thread_id];
                            thread_map.erase(*sibling_thread_id);
                            thread_num--;
                        }
                        else {
                            // If this sibling thread still has live children, then THEY
                            // must delete it.
                            // We'll leave it around for its last child to delete.
                            sibling_thread->sibling_skipped_kill = true;
                        }
                    }
                }
            }
            (thread_map[this_parent_threadid]->child_data).clear();

            if (thread_map[this_parent_threadid]->sibling_skipped_kill) {
                // If one of our parents siblings skipped killing it because we (the child) existed
                // We, the last child, must be the one to kill it.
                // (This also includes the one parent one child case, where it skips killing itself in the sibling-list walk).
                thread_wp_data_t *parent_thread = thread_map[this_parent_threadid];;
                delete parent_thread->self_mem_ptr;
                delete thread_map[this_parent_threadid];
                thread_map.erase(this_parent_threadid);
                thread_num--;
            }
        }
    }
    ReleaseLock(&init_lock);
}

// This would check for read watchfault. And save it to mem(as read set) if there are any.
VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)
{
    if (thread_num > 1) {
        GetLock(&init_lock, threadid+1);
        OS_THREAD_ID this_threadid = PIN_GetTid();
        thread_wp_data_t* this_thread = thread_map[this_threadid];
        if ( (this_thread->wp).read_fault( (ADDRINT) (addr), (ADDRINT) (size) ) ) {
            (this_thread->wp).rm_read ((ADDRINT) (addr), (ADDRINT) (size) );
            (this_thread->self_mem_ptr->mem).add_read_wp((ADDRINT) (addr), (ADDRINT) (size) );
        }
        ReleaseLock(&init_lock);
    }
    return;
}

// This would check for write watchfault. And save it to mem(as read set) if there are any.
VOID RecordMemWrite(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)//, THREADID threadid)
{
    if (thread_num > 1) {
        GetLock(&init_lock, threadid+1);
        OS_THREAD_ID this_threadid = PIN_GetTid();
        thread_wp_data_t* this_thread = thread_map[this_threadid];
        if ( (this_thread->wp).write_fault((ADDRINT) (addr), (ADDRINT) (size) ) ) {
            (this_thread->wp).rm_write ((ADDRINT) (addr), (ADDRINT) (size) );
            (this_thread->self_mem_ptr->mem).add_write_wp((ADDRINT) (addr), (ADDRINT) (size) );
        }
        ReleaseLock(&init_lock);
    }
    return;
}

// This function is called before every block
VOID PIN_FAST_ANALYSIS_CALL docount(ADDRINT c)
{
    thread_map[PIN_GetTid()]->self_mem_ptr->total_instructions += c;
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
                BBL_NumIns(bbl), IARG_END);
    }
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
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
    // Write to a file since cout and cerr maybe closed by the application
    ofstream OutFile;
    OutFile.open(KnobOutputFile.Value().c_str());
    OutFile << "Total number of instructions: " << root_mem_data.total_instructions << endl;

    OutFile << endl << "**Trie data:" << endl;
    OutFile << "The number of total top-level accesses: " << root_mem_data.trie.top_hit << endl;
    OutFile << "The number of total mid-level accesses: " << root_mem_data.trie.mid_hit << endl;
    OutFile << "The number of total bot-level accesses: " << root_mem_data.trie.bot_hit << endl;
    OutFile << "The number of total top-level changes: " << root_mem_data.trie.top_change << endl;
    OutFile << "The number of total mid-level changes: " << root_mem_data.trie.mid_change << endl;
    OutFile << "The number of total bot-level changes: " << root_mem_data.trie.bot_change << endl;
    OutFile << "The number of total top-level breaks: " << root_mem_data.trie.top_break << endl;
    OutFile << "The number of total mid-level breaks: " << root_mem_data.trie.mid_break << endl;
    OutFile << "Notes*: *break* means a top or second level entry can't represent the whole page below anymore." << endl << endl;

    OutFile << "The number of total top-level WLB hits: " << root_mem_data.trie.wlb_hit_top << endl;
    OutFile << "The number of total mid-level WLB hits: " << root_mem_data.trie.wlb_hit_mid << endl;
    OutFile << "The number of total bot-level WLB hits: " << root_mem_data.trie.wlb_hit_bot << endl;
    OutFile << "The number of total top-level WLB misses: " << root_mem_data.trie.wlb_miss_top << endl;
    OutFile << "The number of total mid-level WLB misses: " << root_mem_data.trie.wlb_miss_mid << endl;
    OutFile << "The number of total bot-level WLB misses: " << root_mem_data.trie.wlb_miss_bot << endl << endl;

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
        OutFile << "The number of top-level WLB misses for this thread: " << trie_iter->wlb_miss_top << endl;
        OutFile << "The number of mid-level WLB misses for this thread: " << trie_iter->wlb_miss_mid << endl;
        OutFile << "The number of bot-level WLB misses for this thread: " << trie_iter->wlb_miss_bot << endl;
        OutFile << "----" << endl;
    }
   
#ifdef RANGE_CACHE 
    OutFile << endl << "**Range_cache data: " << endl;
    OutFile << "The average number of ranges in the system: " << ((double)root_mem_data.range.total_cur_range_num/root_mem_data.range.changes) << endl;
    OutFile << "The number of range hits: " << root_mem_data.range.hit << endl;
    OutFile << "The number of range misses: " << root_mem_data.range.miss << endl;
    OutFile << "The number of ranges kicked out: " << root_mem_data.range.kick << endl;
    OutFile << "The number of times dirty ranges were kicked: " << root_mem_data.range.dirty_kick << endl << endl;
    
    deque<unsigned long long>::iterator iter;
    for (iter = total_max_range_num.begin(); iter != total_max_range_num.end(); iter++) {
        OutFile << "The max_range_num for this thread is: " << *iter << endl;
    }
    
    OutFile << endl;

    deque<range_data_t>::iterator range_iter;
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
    OutFile << endl << "**PageTable data: " << endl;
    OutFile << "The number of accesses to a page marked as watched: " << root_mem_data.pagetable.access << endl;
    OutFile << "The number of accesses to a real watchpoint: " << root_mem_data.pagetable.wp_hit << endl;
        OutFile << endl;

    deque<pagetable_data_t>::iterator pagetable_iter;
    for (pagetable_iter = total_pagetable_data.begin(); pagetable_iter != total_pagetable_data.end(); pagetable_iter++) {
        OutFile << "The number of accesses to pages marked in this thread: " << pagetable_iter->access << endl;
        OutFile << "The number of accesses to real watchpoint in this thread: " << pagetable_iter->wp_hit << endl;
        OutFile << "----" << endl;
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
    cerr << "Grace Watchpoint system." << endl;
    cerr << "  Just give this guy an fork/join program to run." << endl;
    cerr << "NOTE: This will not work with LibGomp OpenMP programs ";
    cerr << "because they don't kill the worker threads at the end of ";
    cerr << "each parallel region.  Use grace_omp.so for them." << endl;
    cerr << "Will give output data in grace_tls.out unless you give ";
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
