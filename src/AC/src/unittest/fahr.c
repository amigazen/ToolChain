main()
{
    int fahr, lower, upper, step;
    float celsius;

    lower = 0;
    upper = 300;
    step = 20;

    fahr = lower;
    while (fahr <= upper)    {
        celsius = (5.0/9.0) * (fahr-32.0);
        printf("%4d %6.1f\n", fahr, celsius);
        fahr = fahr + step;
        }
}
