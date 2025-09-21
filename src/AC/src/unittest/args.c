#include <stdio.h>

main(argc, argv)
int argc;
char *argv[];	
{
    int i;
    extern int Enable_Abort;

    Enable_Abort = 1;

    Delay(50L);

    printf("Enable_Abort = %ld\n", Enable_Abort);
    printf("argc: %d\n", argc);
	
    if (argc)
	for (i=0; i<argc; i++)
	    printf("argv[%ld]: %s\n", i, argv[i]);

    return(0);
}
