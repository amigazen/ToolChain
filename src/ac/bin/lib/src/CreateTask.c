#include <exec/memory.h>
#include <exec/tasks.h>

struct Task 
*CreateTask (myTaskName, myTaskPriority, task_EntryPoint, task_stacksize)
char *myTaskName;
UBYTE myTaskPriority;
APTR task_EntryPoint;
ULONG task_stacksize;
{
    struct Task *myTask;
    ULONG dataSize = (task_stacksize & 0x0FFFFFC) + 1;

    myTask = AllocMem (sizeof (*myTask) + dataSize, MEMF_CLEAR|MEMF_PUBLIC );

    if (myTask != 0L) {
        myTask->tc_SPLower = (APTR) (myTask + sizeof(*myTask));
        myTask->tc_SPUpper = (APTR) ((myTask->tc_SPLower + dataSize) & 0xFFFFFE);
        myTask->tc_SPReg = (APTR) myTask->tc_SPUpper;
        myTask->tc_Node.ln_Type = NT_TASK;
        myTask->tc_Node.ln_Pri = myTaskPriority;
        myTask->tc_Node.ln_Name = myTaskName;

        AddTask (myTask, task_EntryPoint, 0);
        }

    return (myTask);
}
