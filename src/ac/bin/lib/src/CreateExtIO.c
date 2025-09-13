#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <exec/ports.h>

struct IORequest *
CreateExtIO(taskReplyPort, size_extreq)
struct MsgPort *taskReplyPort;
LONG size_extreq;
{
    struct IORequest *myExtReq;

    if (taskReplyPort == 0) {
       myExtReq = 0;
       }
    else {
        myExtReq = AllocMem (size_extreq, MEMF_CLEAR | MEMF_PUBLIC);
        if (myExtReq != 0) {
            myExtReq->io_Message.mn_Node.ln_Type = NT_MESSAGE;
            myExtReq->io_Message.mn_Node.ln_Pri = 0;
            myExtReq->io_Message.mn_ReplyPort = taskReplyPort;
            }
        }

    return (myExtReq);
}


