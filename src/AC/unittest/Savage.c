
#define MaxLimit    1

extern double   tan(), atan(), log(), exp(), sqrt();

double          a, b, c, d, e;

main()
{
    int             iter, i, iloop;

    for (iter = 0; iter < MaxLimit; iter++) {
        a = 1.0;
        iloop = 2499;

        for (i = 0; i < iloop; i++) {
            a = tan(atan(exp(log(sqrt(a * a))))) + 1.0;
            printf("the value of 'a' = %24.20e\n", a);
        }
    }
}
