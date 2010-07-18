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

using std::deque;
using Hongyi_WatchPoint::WatchPoint;
using Hongyi_WatchPoint::trie_data_t;

//My own data
struct thread_wp_data_t
{
	OS_THREAD_ID		parent_threadid;//this points to its parent thread
	deque<OS_THREAD_ID>	child_threadid;//this points to all of its child thread. when child_thread_id.size() = 0, then this thread become a leaf.
	bool			root;//this tells whether if the the thread is root. Well if it's the root thread then parent_thread_id would be invalid.
	trie_data_t		trie;//this holds the trie data from itself and all its children's 
	MEM_WatchPoint<ADDRINT, UINT32> mem_commit;//This is for for those commited read/write sets.
	MEM_WatchPoint<ADDRINT, UINT32> mem;//This is the read/write sets that being temperal.
	WatchPoint<ADDRINT, UINT32> wp;
};

INT32 thread_num = 0;//Well this is the only thing to determine whether if a thread is a root or not. If thread_num = 0. Then the thread is a root.

map<OS_THREAD_ID,thread_wp_data_t*>				thread_map;
map<OS_THREAD_ID,thread_wp_data_t*>::iterator	thread_map_iter;

trie_data_t trie_total;
//My own data
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "Grace_tls.out", "specify output file name");

PIN_LOCK init_lock;

// Force each thread's data to be in its own data cache line so that
// multiple threads do not contend for the same data cache line.
// This avoids the false sharing problem.
#define PADSIZE 56  // 64 byte line size: 64-8
#define MEM_SIZE	-1	// 0xffffffff as the max vertual memory address.

VOID StageCommit() {//StageCommit don't have lock within. It must be called within functions which had already locked the data.
	thread_wp_data_t* object_thread;
	for (thread_map_iter = thread_map.begin(); thread_map_iter != thread_map.end(); thread_map_iter++) {
		object_thread = thread_map_iter->second;//cast the iterator's data part to object_thread
		watchpoint_t temp;//temp will hold the watchpoint_t struct dumped out from object_thread
		if (!object_thread->root) {
			(object_thread->mem).DumpStart();//This function is not implemented yet. It would get read to dump all the "read/write set" out;
			while (!(object_thread->mem).DumpEnd() ) {
				temp = (object_thread->mem).Dump();//Not implemented yet. This would dump out all the wp one by one.
				thread_wp_data_t* commit_thread = object_thread;//this is an iterator that will go through all the trunks until reaches the root. Start from *this thread.
				while (!commit_thread->root) {//As long as it doesn't reaches the root.
					(commit_thread->mem_commit).add_wp_struct(temp);//Not implemented yet. This would add the temp into parent thread's commit_mem;
					commit_thread = thread_map[commet_thread->parent_threadid];//Go to higher parent's thread.
				}
			}
		}
		object_thread->trie = object_thread->trie + (object_thread->wp).get_trie_data();//Add the trie_fault data into the tree.
		(object_thread->wp).add_watch_wp(0, MEM_SIZE);//put the watchpoint back on;
		(object_thread->wp).reset_trie();//Not implemented yet. Reset the trie fault in wp.
		(object_thread->mem).clear();//clear out all the "readwrite sets" as they are commited.
	}
}
				
bool thread_commit_data_conflict(MEM_WatchPoint<ADDRINT, UINT32>& sibling_mem, MEM_WatchPoint<ADDRINT, UINT32>& this_mem) {
	watchpoint_t temp;//temp will hold the watchpoint_t struct dumped out from object_thread
	(this_mem).DumpStart();//This function is not implemented yet. It would get read to dump all the "read/write set" out;
	while (!(this_mem).DumpEnd() ) {
		temp = (this_mem).Dump();//Not implemented yet. This would dump out all the wp one by one.
		if (temp->flags & WA_WRITE) {//If this is a write, then check both read and write.
			if (sibling_mem.watch_fault(temp.addr, temp.size) )
				return true;
		}
		if (temp->flags & WA_READ) {//If this is a read, then check write.
			if (sibling_mem.write_fault(temp.addr, temp.size) )
				return true;
		}
	}
	return false;
}	

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
	GetLock(&init_lock, threadid+1);//get LOCK
	thread_wp_data_t* this_thread = new thread_wp_data_t;
	if (thread_num == 0)
		this_thread->root = true;
	else {
		this_thread->root = false;
		this_thread->parent_threadid = PIN_GetParentTid();
		StageCommit();//If not the root starting, then there will be a stage commit.
	}
	(this_thread->wp).add_watch_wp(0, MEM_SIZE);
	
	OS_THREAD_ID this_threadid = Pin_GetTid();
	thread_map[this_threadid] = this_thread;
	thread_num++;
		
	ReleaseLock(&init_lock);//release lOCK
}

VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
	OS_THREAD_ID this_threadid = Pin_GetTid();
	thread_wp_data_t* this_thread = thread_map[this_threadid];
	while (1) {
		GetLock(&init_lock, threadid+1);//get LOCK
		if (this_thread->child_threadid.size() == 0)//Only when it becomes a leaf and has no child threads, can this thread ends. 
			break;
		ReleaseLock(&init_lock);//release lOCK
	}
    StageCommit();//First this is a commit stage as a thread is going to end.
    if (this_thread->root) {//If this thread is a root thread.
    	trie_total = trie_total + this_thread->trie;//just output the total thread.
    else {//If this is not the root thread.
    	parent_thread = thread_map[this_thread->parent_threadid];
    	parent_thread->trie = parent_thread->trie + this_thread->trie;//first add its trie_fault into its parent's.
    	deque<OS_THREAD_ID>::iterator sibling_iter;//It will iter through all its siblings to check W+W,W+R,R+W faults.
    	thread_wp_data_t* sibling_thread;//the data pointer of sibling thread
    	for (sibling_iter = (parent_thread->child_threadid).begin(); sibling_iter != (parent_thread->child_threadid).end(); sibling_iter++) {
    		if (*sibling_iter != this_thread_id) {//sibling must not be itself
    			sibling_thread = thread_map[*sibling_iter];
    			if (thread_commit_data_conflict(sibling_thread->mem_commit, this_thread-mem_commit) )//If there are any W+W,W+R,R+W conflicts between its mem_commit and its siblings'
    				parent_thread->trie = parent_thread->trie + this_thread->trie;
    		}
    	}
    }
    
	delete thread_map[this_threadid];
	thread_map.erase (this_threadid);
	thread_num--;
    ReleaseLock(&init_lock);//release LOCK
}

// This would check for read watchfault. And save it to mem(as read set) if there are any.
VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)
{
	GetLock(&init_lock, threadid+1);//get LOCK
	OS_THREAD_ID this_threadid = Pin_GetTid();
	thread_wp_data_t* this_thread = thread_map[this_threadid];
    if ( (this_thread->wp).read_fault( (ADDRINT) (addr), (ADDRINT) (size) ) ) {
		(this_thread->wp).rm_read ((ADDRINT) (addr), (ADDRINT) (size) );
		(this_thread->mem).add_read_wp((ADDRINT) (addr), (ADDRINT) (size) );
	}
	ReleaseLock(&init_lock);//release LOCK
	return;
}

// This would check for write watchfault. And save it to mem(as read set) if there are any.
VOID RecordMemWrite(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)//, THREADID threadid)
{
	GetLock(&init_lock, threadid+1);//get LOCK
	OS_THREAD_ID this_threadid = Pin_GetTid();
	thread_wp_data_t* this_thread = thread_map[this_threadid];
    if ( (this_thread->wp).write_fault((ADDRINT) (addr), (ADDRINT) (size) ) ) {
		(this_thread->wp).rm_write ((ADDRINT) (addr), (ADDRINT) (size) );
		(this_thread->mem).add_write_wp((ADDRINT) (addr), (ADDRINT) (size) );
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

	// Initialize data
	DataInit();

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
