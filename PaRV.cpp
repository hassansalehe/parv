#include <stdio.h>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <assert.h>
#include <time.h>
#include <sys/unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>
#include <iomanip>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#include "localMems.hpp"
#include "parv_base.hpp"
// * Instruments a function a thread executes as its body *
// * Inserts STM initialization and stm begin transation at
//     the beginning of thread body execution *
// * Inserts STM finalization and stm end transation at the
//     end so that all the body encapsulated *
// * within a transactional context *
static void SpecialInstruction(const char * type, ADDRINT id) {
  if (id != 0) {
        ///printf("App Alive %u %s\n", id, type); fflush(stdout);
    }
}
static void instrumentThreadBody(ADDRINT functptr) {
    PIN_LockClient();
    map<ADDRINT, ADDRINT>::iterator it = instrumentedFuncs.find(functptr);
    CONTEXT ctct;
    if (it == instrumentedFuncs.end()) { // is it not instrumented before?
        printf("THREAD_CREATE\n");  fflush(stdout);
        //register the function
        instrumentedFuncs[functptr] = functptr;

        RTN rtn = RTN_FindByAddress(functptr);
        RTN_Open(rtn);
        INS ins = RTN_InsHead(rtn);

        for (; !INS_Valid(ins); ins = INS_Next(ins))
            ;
        //ins = INS_Next(ins);

        if (INS_Valid(ins)) {
            // insert context initialization function
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)Begin_Consistency,
                    IARG_PTR, "Polka",
                    IARG_PTR, "invis-eager",
                    IARG_BOOL, true,
                    IARG_THREAD_ID,
                    IARG_END);

//             INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(PIN_SaveContext),
//                     IARG_CONTEXT,
//                     IARG_PTR, &ctct, /*IARG_PTR, &threadLockalData[IARG_THREAD_ID].jumpBuf,*/
//                     IARG_END);
            // Instert a function to start a consistency region
            INS_InsertCall(INS_Next(ins), IPOINT_BEFORE, AFUNPTR(Begin_Consistency_Block),
                    IARG_THREAD_ID,
                    IARG_CONTEXT,
                    IARG_INST_PTR,
                    IARG_END);
        }
        // inserted at a place to be executed just before a thread returns in order to encapsulate everything into blocks
        RTN_InsertCall(rtn, IPOINT_AFTER, AFUNPTR(End_Consistency_Block),
                IARG_THREAD_ID,
                IARG_INST_PTR,
                IARG_END);

        //for shutting down consistency before the thread returns
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)Shut_Down_Consistency,
                IARG_END);

        RTN_Close(rtn);
    }
    PIN_UnlockClient();
}


// * for instrumenting an image whenever loaded *
// * Partition into consistency blocks based on presence of *
// * pthread library function calls *
VOID ImageLoad(IMG img, VOID *) {

    // this image already loaded?
    if (pthreadLoaded) return;

    // Instrumenting "main function"
    RTN rtn = RTN_FindByName(img, "__pthread_create_2_1");
    if (RTN_Valid(rtn)) {
        // pthread_create uses internal lock which is instrumented
        // we don't have to end a block here because end_consistency_block
        //  will be called once that lock is come across.
        RTN_Open(rtn);

        RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(instrumentThreadBody),
                IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                IARG_THREAD_ID,
                IARG_INST_PTR,
                IARG_END);

        RTN_Close(rtn);
    }
}


// * Its a callback run everytime an image is unloaded *
VOID ImageUnload(IMG img, VOID *) {
    if (pthreadLoaded && IMG_Id(img) == pthreadIMGID) {
        pthreadLoaded = false;
        pthreadIMGID = 0;
    }
}


// * Its a callback executed to in order to instrument a trace *
void TraceAnalysisCalls(TRACE trace, void *) {
    if (!filter.SelectTrace(trace))
        return;

    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {

            //RTN rtn = TRACE_Rtn(trace);

            //if ( RTN_Valid(rtn) && IMG_IsMainExecutable( SEC_Img( RTN_Sec(rtn) ) ) )
            //{
                if ( INS_IsOriginal(ins) &&  !INS_IsStackRead(ins) && !INS_IsStackWrite(ins)
                    && (INS_IsMemoryRead(ins) || INS_IsMemoryWrite(ins) || INS_HasMemoryRead2(ins)) )
                {
    char * op1;
    char * op2;

    if (INS_OperandIsMemory(ins, 0))       op1 = "MEMORY";
    if (INS_OperandIsReg(ins, 0))          op1 = "REG";
    if (INS_OperandIsImmediate(ins, 0))    op1 = "IMMEDIATE";

    if (INS_OperandIsMemory(ins, 1))       op2 = "MEMORY";
    if (INS_OperandIsReg(ins, 1))          op2 = "REG";
    if (INS_OperandIsImmediate(ins, 1))    op2 = "IMMEDIATE";

    if (INS_IsMemoryRead(ins)) {
        UINT32 memOperands = INS_OperandCount(ins);

        if (PIN_ThreadId())
        printf("===========REading: %s, %s %u\n", op1, op2, PIN_ThreadId());
//         else
//             printf("REading: %s, %s %u\n", op1, op2, PIN_ThreadId());
        if (INS_OperandIsMemory(ins, 1)) {
//             INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(Sibling_Read),
//                     IARG_FAST_ANALYSIS_CALL,
//                     IARG_MEMORYREAD_EA,
//                     IARG_RETURN_REGS,
//                     REG_INST_G0+0,
//                     IARG_END);
//
//             INS_RewriteMemoryOperand(ins, 0, REG(REG_INST_G0+0));
        } else if (INS_OperandIsReg(ins, 0)) {

        } else {
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SpecialInstruction),IARG_PTR, "Reading", IARG_THREAD_ID, IARG_END);
            //printf("(INS_IsMemoryRead(ins))  UNHANDLED MEMORY UNHANDLED MEMORY UNHANDLED MEMORY op1:%s  op2:%s\n", op1, op2); fflush(stdout);
            // A special memory read operation. It may relate to stack read operations
        }
    }
    if (INS_IsMemoryWrite(ins)) {
        UINT32 memOperands = INS_OperandCount(ins);

        if (PIN_ThreadId())
        printf("~~~~~~~~~~~~~~~~~WRiting: %s, %s %u\n", op1, op2, PIN_ThreadId());

        ADDRINT op_2;

        if (INS_OperandIsImmediate(ins, 1)) {
//             op_2 = (ADDRINT)INS_OperandImmediate(ins, 1);
//             INS_InsertCall(ins, IPOINT_BEFORE , AFUNPTR(Sibling_Write),
//                     IARG_FAST_ANALYSIS_CALL,
//                     IARG_MEMORYWRITE_EA,
//                     IARG_ADDRINT, &op_2,
//                     IARG_END);
//
//             INS_InsertDirectJump(ins, IPOINT_BEFORE, INS_NextAddress(ins));

        } else if (INS_OperandIsReg(ins, 1)) {
            //op_2 = ;
//                             INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(Sibling_Write),
//                                     IARG_FAST_ANALYSIS_CALL,
//                                     IARG_MEMORYWRITE_EA,
//                                     IARG_REG_VALUE, INS_OperandReg(ins, 1),
//                                     IARG_END);
//
//                             INS_InsertDirectJump(ins, IPOINT_BEFORE, INS_NextAddress(ins));
        } else if (INS_OperandIsMemory(ins, 1)) {
//                             INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(Sibling_Write),
//                                     IARG_FAST_ANALYSIS_CALL,
//                                     IARG_MEMORYWRITE_EA,
//                                     IARG_REG_VALUE, INS_OperandReg(ins, 1),
//                                     IARG_END);
//
//                             INS_InsertDirectJump(ins, IPOINT_BEFORE, INS_NextAddress(ins));
        } else {
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SpecialInstruction), IARG_PTR, "Writing", IARG_THREAD_ID, IARG_END);
            //printf("(INS_IsMemoryWrite(ins)) UNHANDLED MEMORY UNHANDLED MEMORY UNHANDLED MEMORY\n"); fflush(stdout);
        }
        //INS_InsertDirectJump(ins, IPOINT_BEFORE, INS_NextAddress(ins));
    }
    if (INS_HasMemoryRead2(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SpecialInstruction), IARG_THREAD_ID, IARG_END);
        //printf(" (INS_HasMemoryRead2(ins)) UNHANDLED MEMORY UNHANDLED MEMORY UNHANDLED MEMORY\n"); fflush(stdout);
    }
  }
            //}
        }
    }
}


VOID Fini(INT32 code, VOID *v) {
    //ShutDownStm();
}


int main(int argc, char * argv[]) {

    if (PIN_Init(argc, argv)) return -1;

    PIN_InitSymbols();
    PIN_AddFiniFunction(Fini, 0);
    IMG_AddInstrumentFunction(ImageLoad, 0);
    IMG_AddUnloadFunction(ImageUnload, 0);
    TRACE_AddInstrumentFunction(TraceAnalysisCalls, 0);

    filter.Activate();
//     int myStdoutFd = dup(1);
//     int myStderrFd = dup(2);
//     int nullFd = open ("/home/hmatar/jambazi.txt",  O_RSYNC | O_RDWR  );
//
//     printf("We love you\n");
//     dup2(nullFd, 1);
//     dup2(nullFd, 2);

    PIN_StartProgram();

//     close(nullFd);
    return 0;
}
