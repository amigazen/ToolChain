/* Test simple boolean expressions */

#if 1 && 1
const char* test1 = "1 && 1 works";
#else
const char* test1 = "1 && 1 failed";
#endif

#if 1 || 0
const char* test2 = "1 || 0 works";
#else
const char* test2 = "1 || 0 failed";
#endif

int main() {
    return 0;
}
