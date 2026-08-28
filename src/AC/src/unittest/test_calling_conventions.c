/* Test file for __regargs and __stdargs calling conventions */

/* Test __regargs - parameters passed in registers */
__regargs int regargs_func(int a, int b, int c) {
    return a + b + c;
}

/* Test __stdargs - parameters passed on stack */
__stdargs int stdargs_func(int a, int b, int c) {
    return a + b + c;
}

/* Test mixed calling conventions */
__regargs int mixed_func1(int a, int b) {
    return a + b;
}

__stdargs int mixed_func2(int a, int b) {
    return a + b;
}

/* Test with different parameter types */
__regargs long regargs_long(long a, long b) {
    return a + b;
}

__stdargs long stdargs_long(long a, long b) {
    return a + b;
}

/* Test with pointers */
__regargs int regargs_ptr(int *a, int *b) {
    return *a + *b;
}

__stdargs int stdargs_ptr(int *a, int *b) {
    return *a + *b;
}

/* Test function pointers with calling conventions */
int (*regargs_ptr_func)(int, int) = regargs_func;
int (*stdargs_ptr_func)(int, int) = stdargs_func;

int main() {
    int result1 = regargs_func(1, 2, 3);
    int result2 = stdargs_func(1, 2, 3);
    int result3 = mixed_func1(10, 20);
    int result4 = mixed_func2(10, 20);
    
    long result5 = regargs_long(100L, 200L);
    long result6 = stdargs_long(100L, 200L);
    
    int x = 5, y = 10;
    int result7 = regargs_ptr(&x, &y);
    int result8 = stdargs_ptr(&x, &y);
    
    /* Test function pointers */
    int result9 = regargs_ptr_func(1, 2);
    int result10 = stdargs_ptr_func(1, 2);
    
    return result1 + result2 + result3 + result4 + result5 + result6 + result7 + result8 + result9 + result10;
}
