
#include <stdio.h>

#define size    8190
#define LOOP    100
#define TRUE    1
#define FALSE   0

char            flags[size+1];

main()
{
    int             i, prime, k, count, iter;

    printf("\n%d iterations\n", LOOP);

    for (iter = 1; iter <= LOOP; iter++) {
        count = 0;                          /* prime counter        */
        for (i = 0; i <= size; i++)         /* set all flags TRUE   */
            flags[i] = TRUE;
        for (i = 0; i <= size; i++) {
            if (flags[i]) {                 /* found a prime        */
                prime = i + i + 3;          /* twice index plus 3   */
                k = i + prime;
                for (k = i+prime; k <= size; k += prime) 
                    flags[k] = FALSE;       /* kill all multiples   */
                count++;
            }
        }
    }
    printf("\n%d primes.\n\n", count);
}
