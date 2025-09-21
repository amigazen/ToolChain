#include <exec/memory.h>
#include <exec/ports.h>

DeletePort (myMsgPort)
struct MsgPort *myMsgPort;
{
    if ((myMsgPort->mp_Node.ln_Name) != 0)
         RemPort (myMsgPort);
    myMsgPort->mp_Node.ln_Type = 0xFF;
    myMsgPort->mp_MsgList.lh_Head = -1;
    FreeSignal(myMsgPort->mp_SigBit);
    FreeMem (myMsgPort, sizeof(*myMsgPort));
}

