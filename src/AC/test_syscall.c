/* Test file for syscall pragma functionality */

/* Test syscall pragma - this should generate __SYSCALL_Open calls */
#pragma syscall DOS Open 0 "char *,LONG"

/* Test function that uses syscall */
int test_syscall() {
    char *filename = "test.txt";
    LONG mode = 1005; /* MODE_READWRITE */
    
    /* This should generate a __SYSCALL_Open call */
    LONG file = Open(filename, mode);
    
    if (file) {
        Close(file);
        return 0;
    }
    return 1;
}

/* Test with different syscall pragma */
#pragma syscall DOS Write 0 "LONG,void *,LONG"

int test_write() {
    LONG file = 1; /* stdout */
    char *buffer = "Hello World";
    LONG length = 11;
    
    /* This should generate a __SYSCALL_Write call */
    Write(file, buffer, length);
    
    return 0;
}

/* Test multiple syscall pragmas for same function */
#pragma syscall DOS Close 0 "LONG"

int main() {
    int result1 = test_syscall();
    int result2 = test_write();
    
    return result1 + result2;
}
