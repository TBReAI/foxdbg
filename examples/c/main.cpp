#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <foxdbg.h>

#include <stdint.h>
#include <stdio.h>

#include <windows.h>

#if defined(_MSC_VER)
    #include <windows.h>

    #define YIELD_CPU() Sleep(0)
    #define ATOMIC_READ_INT(ptr) InterlockedCompareExchange((volatile LONG *)(ptr), 0, 0)
    #define ATOMIC_WRITE_INT(ptr, val) InterlockedExchange((volatile LONG *)(ptr), (val))

#elif defined(__GNUC__) || defined(__clang__)
    #include <sched.h>

    #define YIELD_CPU() sched_yield()
    #define ATOMIC_READ_INT(ptr) __atomic_load_n((ptr), __ATOMIC_SEQ_CST)
    #define ATOMIC_WRITE_INT(ptr, val) __atomic_store_n((ptr), (val), __ATOMIC_SEQ_CST)
#else
    #error "Unsupported compiler - implement atomic operations for your compiler"
#endif


bool is_running = true;

static BOOL WINAPI signal_handler(DWORD signal)
{
    is_running = false;
    return TRUE;
}

int main(int argc, char *argv[])
{
    /* Initialize the debugger */
    foxdbg_init();

    int channel_id = foxdbg_add_channel("/sensors/banana", FOXDBG_CHANNEL_TYPE_IMAGE);   

    int width, height, channels;

    uint8_t* data = stbi_load("C:/banana.png", &width, &height, &channels, 3);  

    channels = 3;

    foxdbg_image_info_t image_info;
    image_info.width = width;
    image_info.height = height;
    image_info.channels = channels;

    foxdbg_write_channel_info(channel_id, &image_info, sizeof(image_info));

    printf("Image info: %d x %d, channels: %d, size: %llu\n", 
        image_info.width,
        image_info.height,
        image_info.channels,
        width * height * channels
    );

    SetConsoleCtrlHandler(signal_handler, TRUE);

    foxdbg_write_channel(channel_id, data, width * height * channels);


    while (is_running)
    {
        /* code */
        foxdbg_write_channel(channel_id, data, width * height * channels);

    }
    
    foxdbg_shutdown();

    return 0;
}

    

