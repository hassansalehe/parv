#include "parv_base.hpp"

void static processLocalMem(INS & ins)
{
    char * op1 = "MEMORY";
    char * op2 = "MEMORY";

    if(INS_OperandIsMemory(ins, 0))
        op1 = "MEMORY";
    if(INS_OperandIsReg(ins, 0))
        op1 = "REG";
    if(INS_OperandIsImmediate(ins, 0))
        op1 = "IMMEDIATE";

    if(INS_OperandIsMemory(ins, 1))
        op2 = "MEMORY";
    if(INS_OperandIsReg(ins, 1))
        op2 = "REG";
    if(INS_OperandIsImmediate(ins, 1))
        op2 = "IMMEDIATE";

    if (INS_IsMemoryRead(ins))
    {
        UINT32 memOperands = INS_OperandCount(ins);
        ADDRINT opVal;

        //printf("temp (INS_IsMemoryRead(ins))  MEMORY op1:%s  op2:%s\n", op1, op2); fflush(stdout);
        if(INS_OperandIsMemory(ins, 1))
        {
//          INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(temp_read),
//              IARG_FAST_ANALYSIS_CALL,
//              IARG_MEMORYOP_EA, 0,
//              IARG_RETURN_REGS,
//              REG_INST_G0+0,
//              IARG_END);
//
//          INS_RewriteMemoryOperand(ins, 0, REG(REG_INST_G0+0));

        }else {
            // printf("temp (INS_IsMemoryRead(ins))  UNHANDLED MEMORY UNHANDLED MEMORY UNHANDLED MEMORY op1:%s  op2:%s\n", op1, op2); fflush(stdout);
            // A special memory read operation. It may relate to stack read operations
        }

        if(op1 == "MEMORY" && op2 == "IMMEDIATE")
        {
            opVal = (ADDRINT)INS_OperandImmediate(ins, 1);
//          INS_InsertCall(ins, IPOINT_BEFORE , AFUNPTR(temp_read),
//              IARG_FAST_ANALYSIS_CALL,
//              IARG_MEMORYWRITE_EA,
//              IARG_ADDRINT, &op2,
//              IARG_END);
//
//          INS_InsertDirectJump(ins, IPOINT_BEFORE, INS_NextAddress(ins));
            printf("temp (INS_IsMemoryRead(ins))  MEMORY op1:%s  op2:%s =%d\n", op1, op2, opVal); fflush(stdout);
        }

        else if(op1 == "MEMORY" && op2 == "REG")// (INS_OperandIsReg(ins, 1))
        {
            //op2 = ;
//          INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(temp_read),
//              IARG_FAST_ANALYSIS_CALL,
//              IARG_MEMORYWRITE_EA,
//              IARG_REG_VALUE, INS_OperandReg(ins, 1),
//              IARG_END);
//
//          INS_InsertDirectJump(ins, IPOINT_BEFORE, INS_NextAddress(ins));
            printf("temp (INS_IsMemoryRead(ins))  MEMORY op1:%s  op2:%s = =%d\n", op1, op2, INS_OperandReg(ins, 1)); fflush(stdout);
        }

        else if(op1 == "MEMORY" && op2 == "MEMORY")//else if(INS_OperandIsMemory(ins, 1))
        {
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(temp_read),
                IARG_FAST_ANALYSIS_CALL,
                IARG_MEMORYWRITE_EA,
                IARG_REG_VALUE, INS_OperandReg(ins, 1),
                IARG_ADDRINT, op1, IARG_ADDRINT, op2,
                IARG_END);
//
//          INS_InsertDirectJump(ins, IPOINT_BEFORE, INS_NextAddress(ins));
        }
        else {
            printf(" TEMP (INS_IsMemoryRead(ins)) op1:%s   op2:%s\n", op1, op2); fflush(stdout);
        }
    }
    if (INS_IsMemoryWrite(ins))
    {
        UINT32 memOperands = INS_OperandCount(ins);

        ADDRINT opVal;

        if(op1 == "MEMORY" && op2 == "IMMEDIATE")
        {
            opVal = (ADDRINT)INS_OperandImmediate(ins, 1);
//          INS_InsertCall(ins, IPOINT_BEFORE , AFUNPTR(temp_write),
//              IARG_FAST_ANALYSIS_CALL,
//              IARG_MEMORYWRITE_EA,
//              IARG_ADDRINT, &op2,
//              IARG_END);
//
//          INS_InsertDirectJump(ins, IPOINT_BEFORE, INS_NextAddress(ins));
            ///printf("temp (INS_IsMemoryWrite(ins))  MEMORY op1:%s  op2:%s =%d\n", op1, op2, opVal); fflush(stdout);
        }

        else if(op1 == "MEMORY" && op2 == "REG")// (INS_OperandIsReg(ins, 1))
        {
            //op2 = ;
//          INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(temp_write),
//              IARG_FAST_ANALYSIS_CALL,
//              IARG_MEMORYWRITE_EA,
//              IARG_REG_VALUE, INS_OperandReg(ins, 1),
//              IARG_END);
//
//          INS_InsertDirectJump(ins, IPOINT_BEFORE, INS_NextAddress(ins));
            printf("temp (INS_IsMemoryWrite(ins))  MEMORY op1:%s  op2:%s = =%d\n", op1, op2, INS_OperandReg(ins, 1)); fflush(stdout);
        }

        else if(op1 == "MEMORY" && op2 == "MEMORY")//else if(INS_OperandIsMemory(ins, 1))
        {
//          INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(temp_write),
//              IARG_FAST_ANALYSIS_CALL,
//              IARG_MEMORYWRITE_EA,
//              IARG_REG_VALUE, INS_OperandReg(ins, 1),
//              IARG_END);
//
//          INS_InsertDirectJump(ins, IPOINT_BEFORE, INS_NextAddress(ins));
            printf("temp (INS_IsMemoryWrite(ins))  MEMORY op1:%s  op2:%s = = =%d\n", op1, op2, INS_OperandReg(ins, 1)); fflush(stdout);
        }
        else {
            printf("TEMP (INS_IsMemoryWrite(ins)) op1:%s   op2:%s\n", op1, op2); fflush(stdout);
        }
        //INS_Delete(ins);
    }
}

