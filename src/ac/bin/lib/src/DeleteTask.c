#include <exec/tasks.h>
#include <exec/memory.h>

DeleteTask (myTask)
struct Task *myTask;
{
    RemTask(myTask);
    FreeMem(myTask, 1 + (ULONG) myTask->tc_SPUpper - (ULONG)myTask);
}

