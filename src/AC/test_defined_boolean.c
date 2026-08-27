/* Test defined() with boolean operators */

#define TEST_MACRO 1

#if defined(TEST_MACRO) && defined(__SASC)
const char* test1 = "TEST_MACRO && __SASC";
#else
const char* test1 = "not TEST_MACRO && __SASC";
#endif

#if defined(TEST_MACRO) || defined(__SASC)
const char* test2 = "TEST_MACRO || __SASC";
#else
const char* test2 = "not TEST_MACRO || __SASC";
#endif

int main() {
    return 0;
}
