/* Test file to verify libcall pragma format compatibility */

/* This should be parsed with reversed args to match SAS/C format */
#pragma libcall IntuitionBase OpenWindow 0x4e 0x1902

/* Test function that would use the pragma */
void test_function() {
    /* This would normally call OpenWindow with the pragma above */
    /* The pragma should be parsed with args reversed: 0x1902 -> 0x2091 */
}
