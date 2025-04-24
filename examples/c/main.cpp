#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <foxdbg.h>

#include <stdint.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
    /* Initialize the debugger */
    foxdbg_init();


    getchar();


    foxdbg_shutdown();

    return 0;
}

    

