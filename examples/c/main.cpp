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
    printf("Signal %d received, shutting down...\n", signal);
    is_running = false;
    return TRUE;
}

int main(int argc, char *argv[])
{
    /* Initialize the debugger */
    foxdbg_init();

    int channel_id = foxdbg_add_channel("/sensors/banana", FOXDBG_CHANNEL_TYPE_IMAGE, 30);   
    int channel_id2 = foxdbg_add_channel("/sensors/banana2", FOXDBG_CHANNEL_TYPE_IMAGE, 30);

    int channel_id3 = foxdbg_add_channel("/waves/sin", FOXDBG_CHANNEL_TYPE_FLOAT, 30);
    int channel_id4 = foxdbg_add_channel("/waves/bool", FOXDBG_CHANNEL_TYPE_BOOLEAN, 30);
    int channel_id5 = foxdbg_add_channel("/waves/int", FOXDBG_CHANNEL_TYPE_INTEGER, 30);

    int rx_channel = foxdbg_add_rx_channel("/rx/system_state", FOXDBG_CHANNEL_TYPE_BOOLEAN);

    int width, height, channels;

    uint8_t* data = stbi_load("C:/banana.png", &width, &height, &channels, 3);  
    channels = 3;

    foxdbg_image_info_t image_info;
    image_info.width = width;
    image_info.height = height;
    image_info.channels = channels;

    foxdbg_write_channel_info(channel_id, &image_info, sizeof(image_info));

    int width2, height2, channels2;
    uint8_t* data2 = stbi_load("examples/c/banana.jpg", &width2, &height2, &channels2, 3);
    channels2 = 3;

    foxdbg_image_info_t image_info2;
    image_info2.width = width2;
    image_info2.height = height2;
    image_info2.channels = channels2;

    foxdbg_write_channel_info(channel_id2, &image_info2, sizeof(image_info2));

    SetConsoleCtrlHandler(signal_handler, TRUE);

    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);

    while (is_running)
    {
        /* code */
        foxdbg_write_channel(channel_id, data, width * height * channels);
        foxdbg_write_channel(channel_id2, data2, width2 * height2 * channels2);

        QueryPerformanceCounter(&start);

        float sin_value = sinf((float)start.QuadPart / (float)frequency.QuadPart * 2.0f * 3.14159f * 0.1f);
        foxdbg_write_channel(channel_id3, &sin_value, sizeof(sin_value));

        bool is_true = (sin_value > 0.0f);
        foxdbg_write_channel(channel_id4, &is_true, sizeof(bool));

        static int int_value = 0;
        int_value++;

        foxdbg_write_channel(channel_id5, &int_value, sizeof(int_value));


        YIELD_CPU();
    }
    
    foxdbg_shutdown();

    return 0;
}

    

