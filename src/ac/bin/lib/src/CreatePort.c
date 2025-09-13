#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>

struct MsgPort *
CreatePort (msgPortName, msgport_priority)
char *msgPortName;
BYTE  msgport_priority;
{
     UBYTE msgport_signalbitnumber;
     struct MsgPort *myMsgPort;

     if ((msgport_signalbitnumber = AllocSignal(-1)) == -1)
          return (0);

     myMsgPort = AllocMem(sizeof (*myMsgPort), MEMF_CLEAR|MEMF_PUBLIC);
     if (myMsgPort == 0) {
          FreeSignal (msgport_signalbitnumber);
          }
     else {
         myMsgPort->mp_Node.ln_Name = msgPortName;
         myMsgPort->mp_Node.ln_Pri = msgport_priority;
         myMsgPort->mp_Node.ln_Type = NT_MSGPORT;
         myMsgPort->mp_Flags = PA_SIGNAL;
         myMsgPort->mp_SigBit = msgport_signalbitnumber;
         myMsgPort->mp_SigTask = FindTask (0);
         if (msgPortName != 0)
              AddPort (myMsgPort);
         else
              NewList (&(myMsgPort->mp_MsgList));
         }

     return (myMsgPort);
}



