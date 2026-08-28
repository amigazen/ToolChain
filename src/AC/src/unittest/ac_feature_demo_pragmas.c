/*
 * ac_feature_demo_pragmas.c - compile-only SAS/C #pragma coverage for AC 4.0.
 * No Amiga library calls are made; pragmas must parse and codegen must link.
 */

#pragma libcall IntuitionBase OpenWindow 0x4e 0x1902
#pragma libcall IntuitionBase CloseWindow 0x54 0x801

#pragma flibcall MathBase SPAdd 0x1e 0x1002
#pragma flibcall MathBase SPMul 0x24 0x1002

#pragma syscall ExecBase FindTask 0x1e 0x801
#pragma syscall ExecBase AddTask 0x24 0x9802

#pragma tagcall IntuitionBase NewObject 0x1e 0x9802
#pragma tagcall IntuitionBase DisposeObject 0x24 0x801

#pragma msg 85 ignore
#pragma msg 100 warn push
#pragma msg 200 error
#pragma msg 300 pop

/* Anchor symbol so the translation unit is linked when pragmas are compile-only. */
void demo_pragma_anchor(void)
{
    /* Pragma tables are exercised at compile time only. */
}
