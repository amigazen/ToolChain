/* Test file for _Bool support */

#include "stdbool.h"

_Bool global_bool = true;
bool global_bool2 = false;

int main() {
    _Bool local_bool = 1;
    bool local_bool2 = 0;
    
    /* Test basic operations */
    _Bool result1 = local_bool && global_bool;
    bool result2 = local_bool2 || global_bool2;
    
    /* Test conversions */
    int int_val = local_bool;  /* _Bool to int */
    _Bool bool_val = int_val;  /* int to _Bool */
    
    /* Test comparisons */
    if (local_bool == true) {
        return 1;
    }
    
    if (global_bool2 != false) {
        return 2;
    }
    
    return 0;
}
