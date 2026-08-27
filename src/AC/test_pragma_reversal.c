/* Test file to verify pragma argument reversal works correctly */

/* These pragmas should have their args field reversed to match SAS/C format */
/* Original: 0x1902 -> Should become: 0x2091 */
#pragma libcall IntuitionBase OpenWindow 0x4e 0x1902

/* Original: 0x81003 -> Should become: 0x30018 */
#pragma flibcall MathBase SPAdd 0x1e 0x81003

/* Original: 0x9802 -> Should become: 0x2089 */
#pragma syscall ExecBase FindTask 0x1e 0x9802

/* Original: 0xa0921806 -> Should become: 0x6081920a */
#pragma tagcall IntuitionBase NewObject 0x1e 0xa0921806

/* Test function */
void test_pragma_reversal() {
    /* This function would use the pragmas above */
    /* The compiler should internally reverse the args to match SAS/C format */
}
