/*
 * C23-style alias smoke test for AC (Mac host + Amiga).
 */

bool flag;
int8_t i8;
uint32_t u32;
intptr_t ip;
void *np;

_Noreturn void die(void);
inline int square(int x);
int *rp;

int
main()
{
    int32_t a;
    bool ok;

    a = 1;
    ok = true;
    if (false)
        return 1;
    np = nullptr;
    if (np != nullptr)
        return 2;
    if (1 and 0)
        return 3;
    if (not ok)
        return 4;
    i8 = (int8_t)a;
    u32 = (uint32_t)a;
    ip = (intptr_t)&a;
    flag = ok;
    return (int)(a + (int32_t)i8);
}
