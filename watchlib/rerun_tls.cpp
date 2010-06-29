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

#include <iostream>
#include <fstream>
#include "pin.H"
#include "auto_wp.h"

using std::deque;
using Hongyi_WatchPoint::WatchPoint;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "inscount_tls.out", "specify output file name");

PIN_LOCK lock;
INT32 numThreads = 0;

// Force each thread's data to be in its own data cache line so that
// multiple threads do not contend for the same data cache line.
// This avoids the false sharing problem.
#define PADSIZE 56  // 64 byte line size: 64-8
#define MEM_SIZE	-1	// 0xffffffff as the max vertual memory address.

// a running count of the instructions



struct thread_wp_data_t
{
	int threadid;
	WatchPoint<ADDRINT, ADDRINT, UINT32> mem;
	WatchPoint<ADDRINT, ADDRINT, UINT32> wp;
	UINT64 race_count;
	UINT64 watchfault_count;
};

// key for accessing TLS storage in the threads. initialized once in main()
static  TLS_KEY tls_key;

// function to access thread-specific data
thread_wp_data_t* get_tls(THREADID threadid)
{
    thread_wp_data_t* tdata = 
          static_cast<thread_wp_data_t*>(PIN_GetThreadData(tls_key, threadid));
    return tdata;
}

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    GetLock(&lock, threadid+1);
    numThreads++;
    ReleaseLock(&lock);

    thread_wp_data_t* tdata = new thread_wp_data_t;
	
	tdata->threadid = static_cast<int>(threadid);
	tdata->race_count = 0;
	tdata->watchfault_count = 0;
	(tdata->wp).add_watch_wp(0, MEM_SIZE);
	
    PIN_SetThreadData(tls_key, tdata, threadid);
}

// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)
{
	thread_wp_data_t* tdata = get_tls(threadid);
    if ( (tdata->wp).read_fault( (ADDRINT) (addr), (ADDRINT) (size) ) ) {
		(tdata->wp).rm_read ((ADDRINT) (addr), (ADDRINT) (size) );
		(tdata->mem).add_read_wp((ADDRINT) (addr), (ADDRINT) (size) );
		for (INT32 t = 0; t < numThreads; t++) {
			if (t != static_cast<INT32> (threadid) ) {
				tdata = get_tls(t);
				if ( (tdata->mem).write_fault((ADDRINT) (addr), (ADDRINT) (size) ) ) {
					tdata = get_tls(threadid);
					(tdata->mem).clear();
					(tdata->wp).add_watch_wp(0, MEM_SIZE);
					return;
				}
			}
		}
		(tdata->wp).rm_read ((ADDRINT) (addr), (ADDRINT) (size) );
		(tdata->mem).add_read_wp((ADDRINT) (addr), (ADDRINT) (size) );
	}
	return;
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, UINT32 size, THREADID threadid)//, THREADID threadid)
{
    thread_wp_data_t* tdata = get_tls(threadid);
    if ( (tdata->wp).write_fault((ADDRINT) (addr), (ADDRINT) (size) ) ) {
		for (INT32 t = 0; t < numThreads; t++) {
			if (t != static_cast<INT32> (threadid) ) {
				tdata = get_tls(t);
				if ( (tdata->mem).watch_fault((ADDRINT) (addr), (ADDRINT) (size) ) ) {
					tdata = get_tls(threadid);
					(tdata->mem).clear();
					(tdata->wp).add_watch_wp(0, MEM_SIZE);
					return;
				}
			}
		}
		(tdata->wp).rm_write ((ADDRINT) (addr), (ADDRINT) (size) );
		(tdata->mem).add_write_wp((ADDRINT) (addr), (ADDRINT) (size) );
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
    // Write to a file since cout and cerr maybe closed by the application
    ofstream OutFile;
    OutFile.open(KnobOutputFile.Value().c_str());
    OutFile << "Total number of threads = " << numThreads << endl;
    
    for (INT32 t=0; t<numThreads; t++)
    {
        thread_wp_data_t* tdata = get_tls(t);
        OutFile << "thread[" << decstr(t) << "]= " << tdata->threadid << endl;
    }

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

    // Initialize the lock
    InitLock(&lock);

    // Obtain  a key for TLS storage.
    tls_key = PIN_CreateThreadDataKey(0);

    // Register ThreadStart to be called when a thread starts.
    PIN_AddThreadStartFunction(ThreadStart, 0);

    // Register Instruction to be called to instrument instructions.
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits.
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
