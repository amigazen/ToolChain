#include <time.h>

main()
{
    time_t t;
    char *ts;

    t = time(0L);
    ts = ctime(&t);
    write(1, ts, strlen(ts));
}
