
extern double   exp(), log();

double
pow(x, y)
    double          x, y;
{
    double          z;

    z = exp(y * log(x));
    return (z);
}
