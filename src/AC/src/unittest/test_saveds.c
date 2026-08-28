/* Test file for __saveds keyword functionality */

/* Test __saveds function - callback function that preserves registers */
__saveds int saveds_callback(int a, int b) {
    return a + b;
}

/* Test __saveds with different parameter types */
__saveds long saveds_long(long a, long b) {
    return a + b;
}

/* Test __saveds with pointers */
__saveds int saveds_ptr(int *a, int *b) {
    return *a + *b;
}

/* Test __saveds with mixed calling conventions */
__saveds __regargs int saveds_regargs(int a, int b) {
    return a + b;
}

__saveds __stdargs int saveds_stdargs(int a, int b) {
    return a + b;
}

/* Test function pointers with __saveds */
int (*saveds_ptr_func)(int, int) = saveds_callback;

/* Test __saveds in function declarations */
__saveds int external_saveds_func(int a, int b);

int main() {
    int result1 = saveds_callback(10, 20);
    int result2 = saveds_long(100L, 200L);
    
    int x = 5, y = 10;
    int result3 = saveds_ptr(&x, &y);
    
    int result4 = saveds_regargs(1, 2);
    int result5 = saveds_stdargs(1, 2);
    
    /* Test function pointer */
    int result6 = saveds_ptr_func(3, 4);
    
    return result1 + result2 + result3 + result4 + result5 + result6;
}

