/* Comprehensive test suite for 64-bit long long operations */

#include "stdbool.h"

/* Test basic declarations and initialization */
long long global_ll = 123456789012345LL;
unsigned long long global_ull = 987654321098765ULL;

/* Test function with long long parameters */
long long test_add(long long a, long long b) {
    return a + b;
}

unsigned long long test_mul(unsigned long long a, unsigned long long b) {
    return a * b;
}

long long test_div(long long a, long long b) {
    return a / b;
}

long long test_mod(long long a, long long b) {
    return a % b;
}

/* Test conversions */
long long test_conv_int(int i) {
    return (long long)i;
}

int test_conv_to_int(long long ll) {
    return (int)ll;
}

/* Test comparisons */
_Bool test_compare(long long a, long long b) {
    return a == b;
}

_Bool test_less_than(long long a, long long b) {
    return a < b;
}

_Bool test_greater_than(long long a, long long b) {
    return a > b;
}

int main() {
    /* Test local declarations */
    long long local_ll = 1000000000000LL;
    unsigned long long local_ull = 2000000000000ULL;
    
    /* Test arithmetic operations */
    long long sum = test_add(global_ll, local_ll);
    unsigned long long product = test_mul(global_ull, local_ull);
    long long quotient = test_div(global_ll, 1000LL);
    long long remainder = test_mod(global_ll, 1000LL);
    
    /* Test conversions */
    long long conv_from_int = test_conv_int(42);
    int conv_to_int = test_conv_to_int(123456789012345LL);
    
    /* Test comparisons */
    _Bool eq_result = test_compare(global_ll, global_ll);
    _Bool lt_result = test_less_than(local_ll, global_ll);
    _Bool gt_result = test_greater_than(global_ll, local_ll);
    
    /* Test direct operations */
    long long direct_add = global_ll + local_ll;
    unsigned long long direct_mul = global_ull * local_ull;
    long long direct_div = global_ll / 1000LL;
    long long direct_mod = global_ll % 1000LL;
    
    /* Test with constants */
    long long const_add = 123456789012345LL + 987654321098765LL;
    unsigned long long const_mul = 1000000000000ULL * 2000000000000ULL;
    
    /* Test mixed operations */
    long long mixed = (global_ll + local_ll) * 2LL;
    unsigned long long mixed_ull = (global_ull - local_ull) / 1000ULL;
    
    /* Test conditional expressions */
    long long conditional = (global_ll > local_ll) ? global_ll : local_ll;
    unsigned long long conditional_ull = (global_ull < local_ull) ? global_ull : local_ull;
    
    /* Test array operations */
    long long ll_array[5] = {1LL, 2LL, 3LL, 4LL, 5LL};
    unsigned long long ull_array[3] = {10ULL, 20ULL, 30ULL};
    
    long long array_sum = ll_array[0] + ll_array[1] + ll_array[2];
    unsigned long long array_product = ull_array[0] * ull_array[1] * ull_array[2];
    
    /* Test pointer operations */
    long long *ll_ptr = &global_ll;
    unsigned long long *ull_ptr = &global_ull;
    
    long long deref_ll = *ll_ptr;
    unsigned long long deref_ull = *ull_ptr;
    
    /* Test function pointer operations */
    long long (*func_ptr)(long long, long long) = test_add;
    long long func_result = func_ptr(100LL, 200LL);
    
    /* Test complex expressions */
    long long complex = (global_ll + local_ll) * (global_ll - local_ll) / 1000LL;
    unsigned long long complex_ull = (global_ull * global_ull) % (local_ull * local_ull);
    
    /* Test edge cases */
    long long zero = 0LL;
    unsigned long long max_ull = 0xFFFFFFFFFFFFFFFFULL;
    long long min_ll = 0x8000000000000000LL;
    long long max_ll = 0x7FFFFFFFFFFFFFFFLL;
    
    /* Test overflow/underflow (should wrap around) */
    unsigned long long overflow = max_ull + 1ULL;
    long long underflow = min_ll - 1LL;
    
    /* Test bitwise operations (if supported) */
    long long bitwise_and = global_ll & local_ll;
    unsigned long long bitwise_or = global_ull | local_ull;
    long long bitwise_xor = global_ll ^ local_ll;
    long long bitwise_not = ~global_ll;
    
    /* Test shift operations (if supported) */
    long long left_shift = global_ll << 1;
    unsigned long long right_shift = global_ull >> 1;
    
    /* Test unary operations */
    long long unary_minus = -global_ll;
    long long unary_plus = +global_ll;
    
    /* Test assignment operations */
    long long assign_test = 0LL;
    assign_test += global_ll;
    assign_test -= local_ll;
    assign_test *= 2LL;
    assign_test /= 1000LL;
    assign_test %= 100LL;
    
    /* Test increment/decrement (if supported) */
    long long inc_test = global_ll;
    inc_test++;  /* Post-increment */
    ++inc_test;  /* Pre-increment */
    inc_test--;  /* Post-decrement */
    --inc_test;  /* Pre-decrement */
    
    /* Test compound assignments */
    long long compound_test = 1000LL;
    compound_test += global_ll;
    compound_test -= local_ll;
    compound_test *= 2LL;
    compound_test /= 100LL;
    compound_test %= 10LL;
    
    /* Test function calls with long long arguments */
    long long func_add_result = test_add(global_ll, local_ll);
    unsigned long long func_mul_result = test_mul(global_ull, local_ull);
    long long func_div_result = test_div(global_ll, local_ll);
    long long func_mod_result = test_mod(global_ll, local_ll);
    
    /* Test return values */
    return (int)(sum + product + quotient + remainder + conv_from_int + conv_to_int);
}
