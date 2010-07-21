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
using Hongyi_WatchPoint::MEM_WatchPoint;
//My own data
struct thread_wp_data_t
{
	MEM_WatchPoint<ADDRINT, UINT32> mem;
	WatchPoint<ADDRINT, UINT32> wp;
};

map<THREADID,thread_wp_data_t*> thread_map;
map<THREADID,thread_wp_data_t*>::iterator thread_map_iter;

trie_data_t trie_total;
//My own data
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "Rerun_tls.out", "specify output file name");

PIN_LOCK init_lock;

// Force each thread's data to be in its own data cache line so that
// multiple threads do not contend for the same data cache line.
// This avoids the false sharing problem.
#define PADSIZE 56  // 64 byte line size: 64-8
#define MEM_SIZE	0	// 0xffffffff as the max vertual memory address.

// key for accessing TLS storage in the threads. initialized once in main()

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
	GetLock(&init_lock, threadid+1);//get LOCK
	thread_wp_data_t* this_thread = new thread_wp_data_t;

	(this_thread->wp).add_watch_wp(0, MEM_SIZE);
	
	thread_map[threadid] = this_thread;
	ReleaseLock(&init_lock);//release lOCK
}

VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    GetLock(&init_lock, threadid+1);//get LOCK
	trie_total = trie_total + (thread_map[threadid]->wp).get_trie_data();//get data out;
	delete thread_map[threadid];
	thread_map.erase (threadid);
    ReleaseLock(&init_lock);//release LOCK
}

// Print a memory read record
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
//		(this_thread->wp).watch_print();
		ReleaseLock(&init_lock);//release LOCK
	}
	return;
}

// Print a memory write record
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
//		(this_thread->wp).watch_print();
		ReleaseLock(&init_lock);//release LOCK
	}
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
	ofstream OutFile;
	OutFile.open(KnobOutputFile.Value().c_str());
    // Write to a file since cout and cerr maybe closed by the application
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

    // Initialize the init_lock
    InitLock(&init_lock);

    DataInit();

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
