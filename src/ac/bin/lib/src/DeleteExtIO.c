#include <exec/memory.h>
#include <exec/io.h>

DeleteExtIO (myExtReq, size_extreq)
struct IORequest *myExtReq;
LONG size_extreq;
{
    myExtReq->io_Message.mn_Node.ln_Type = 0xFF;
    myExtReq->io_Device = (struct Device *) -1;
    myExtReq->io_Unit = (struct Unit *) -1;
    FreeMem (myExtReq, size_extreq);
}

