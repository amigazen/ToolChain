/* Test file for tagcall pragma functionality */

/* Test tagcall pragma - this should generate __TAGCALL_OpenWindow calls */
#pragma tagcall Intuition OpenWindow 0 "struct Window *,(struct NewWindow *)"

/* Test function that uses tagcall */
int test_tagcall() {
    struct NewWindow *nw = NULL;
    struct Window *w;
    
    /* This should generate a __TAGCALL_OpenWindow call */
    w = OpenWindow(nw);
    
    if (w) {
        CloseWindow(w);
        return 0;
    }
    return 1;
}

/* Test with different tagcall pragma */
#pragma tagcall Graphics DrawText 0 "struct RastPort *,char *,long"

int test_drawtext() {
    struct RastPort *rp = NULL;
    
    /* This should generate a __TAGCALL_DrawText call */
    DrawText(rp, "Hello World", 11);
    
    return 0;
}

/* Test multiple tagcall pragmas for same function */
#pragma tagcall Intuition CloseWindow 0 "struct Window *"

int main() {
    int result1 = test_tagcall();
    int result2 = test_drawtext();
    
    return result1 + result2;
}
