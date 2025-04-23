#include <foxdbg.h>

#include <stdio.h>

int main(int argc, char *argv[])
{
    // Initialize the debugger
    foxdbg_init();

    
    /* wait for user */

    printf("Press Enter to send data...\n");
    getchar();

    foxdbg_shutdown();

    return 0;
}

    

