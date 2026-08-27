/* Test file for flibcall pragma functionality */

/* Test flibcall pragma - this should generate __FLIBCALL_OpenLibrary calls */
#pragma flibcall Exec OpenLibrary 0 "char *,ULONG"

/* Test function that uses flibcall */
int test_flibcall() {
    char *libname = "dos.library";
    ULONG version = 0;
    
    /* This should generate a __FLIBCALL_OpenLibrary call */
    void *lib = OpenLibrary(libname, version);
    
    if (lib) {
        CloseLibrary(lib);
        return 0;
    }
    return 1;
}

/* Test with different flibcall pragma */
#pragma flibcall Graphics LoadRGB4 0 "struct ColorMap *,UBYTE *,WORD"

int test_loadrgb() {
    struct ColorMap *cm = NULL;
    UBYTE *colors = NULL;
    WORD count = 0;
    
    /* This should generate a __FLIBCALL_LoadRGB4 call */
    LoadRGB4(cm, colors, count);
    
    return 0;
}

/* Test multiple flibcall pragmas for same function */
#pragma flibcall Exec CloseLibrary 0 "struct Library *"

int main() {
    int result1 = test_flibcall();
    int result2 = test_loadrgb();
    
    return result1 + result2;
}
