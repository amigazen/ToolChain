/* Test file for long long support */

long long global_ll = 123456789012345LL;
unsigned long long global_ull = 987654321098765ULL;

int main() {
    long long local_ll = 42LL;
    unsigned long long local_ull = 100ULL;
    
    /* Test basic operations */
    long long sum = local_ll + global_ll;
    unsigned long long product = local_ull * global_ull;
    
    /* Test constants with LL/ll suffixes */
    long long const_ll = 1000000000000LL;
    unsigned long long const_ull = 2000000000000ULL;
    
    /* Test mixed operations */
    long long mixed = local_ll + const_ll;
    
    return 0;
}
