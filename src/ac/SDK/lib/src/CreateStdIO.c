#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/io.h>

struct IOStdReq *
CreateStdIO (taskReplyPort)
struct MsgPort *taskReplyPort;
{
    struct IOStdReq *myStdReq;

    if (taskReplyPort == 0) {
        myStdReq = 0;
        }
    else {
        myStdReq = AllocMem (sizeof (*myStdReq), MEMF_CLEAR | MEMF_PUBLIC);
        if (myStdReq != 0) {
            myStdReq->io_Message.mn_Node.ln_Type = NT_MESSAGE;
            myStdReq->io_Message.mn_Node.ln_Pri = 0;
            myStdReq->io_Message.mn_ReplyPort = taskReplyPort;
            }
        }

    return (myStdReq);
}

