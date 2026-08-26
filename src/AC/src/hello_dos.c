/*
 * hello_dos.c ù ac-self2 smoke test (step 0: bare minimum)
 *
 * Goal: prove ac-self2 can compile/link/run anything at all.
 * Add complexity only after this step succeeds:
 *
 *   step 0  (this file)  int main(void) { return 0; }
 *   step 1               local int / return that value
 *   step 2               string literal (no library calls)
 *   step 3               #pragma libcall Output/Write only
 *   step 4               #include <proto/dos.h>  (__SASC path)
 *
 * Build (does not rebuild ac-self2):
 *   make hello-dos
 *   hello_dos
 *
 * Artifacts stay in this directory (hello_dos.s etc.).
 */

int
main(void)
{
    return 0;
}
