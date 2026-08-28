/* Test file to verify complete SAS/C pragma support */

/* Test libcall pragma (library calls) */
#pragma libcall IntuitionBase OpenWindow 0x4e 0x1902
#pragma libcall IntuitionBase CloseWindow 0x54 0x801

/* Test flibcall pragma (floating-point library calls) */
#pragma flibcall MathBase SPAdd 0x1e 0x1002
#pragma flibcall MathBase SPMul 0x24 0x1002

/* Test syscall pragma (system calls) */
#pragma syscall ExecBase FindTask 0x1e 0x801
#pragma syscall ExecBase AddTask 0x24 0x9802

/* Test tagcall pragma (tag-based calls) */
#pragma tagcall IntuitionBase NewObject 0x1e 0x9802
#pragma tagcall IntuitionBase DisposeObject 0x24 0x801

/* Test msg pragma (message control) */
#pragma msg 85 ignore
#pragma msg 100 warn push
#pragma msg 200 error
#pragma msg 300 pop

/* Test functions that would use these pragmas */
void test_libcalls() {
    /* These would normally call the library functions with the pragmas above */
}

void test_flibcalls() {
    /* These would normally call floating-point library functions */
}

void test_syscalls() {
    /* These would normally call system functions */
}

void test_tagcalls() {
    /* These would normally call tag-based functions */
}

/* Test message control */
int test_function() {
    /* This function might trigger warning 85, but it's ignored due to pragma */
    /* Warning 100 is treated as warning with push */
    /* Error 200 is treated as error */
    return 0;
}
