
extern double   sqrt(), pow();

double          a, f, n, r, y;

double
g()
{
    r = sqrt(2.0);
    y = r - 1.0;
    a = 6.0 - 4.0 * r;
    n = 3.0;
    return (1.0 / a);
}

double
i()
{
    double          t1, t2, t3;

    f = sqrt(sqrt(1.0 - y * y * y * y));
    y = (1.0 - f) / (1.0 + f);

    t1 = 1.0 + y;
    t2 = t1 * t1 * t1 * t1;

    a = t2 * a - pow(2.0, n) * y * (1.0 + y + y * y);
    n = n + 2.0;
    return (1.0 / a);
}

main()
{
    double          pi;

    pi = g();
    printf("pi = %20.16e\n", pi);
    pi = i();
    printf("pi = %20.16e\n", pi);
    pi = i();
    printf("pi = %20.16e\n", pi);
    pi = i();
    printf("pi = %20.16e\n", pi);
    pi = i();
    printf("pi = %20.16e\n", pi);
}
