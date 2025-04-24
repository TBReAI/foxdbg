/***************************************************************
**
** TBReAI Source File
**
** File         :  foxdbg_thread.cpp
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server Thread
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "foxdbg.h"
#include "foxdbg_threadpool.h"

#include <libwebsockets.h>
#include <stdio.h>

#include <chrono>
#include <thread>
#include <atomic>


/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static int foxdbg_server_thread_main();
static int foxdbg_encoder_thread_main();

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static std::thread foxdbg_server_thread;
static std::thread foxdbg_encoder_thread;

static std::atomic_bool running(false);

uint8_t tx_buffer[LARGE_BUFFER_SIZE]; /* 1MB tx buffer */
static size_t tx_buffer_size = LARGE_BUFFER_SIZE;

foxdbg_buffer_t buffer = {
    tx_buffer,
    tx_buffer_size,
    false,
    false
};

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void foxdbg_threadpool_init()
{   
    running.store(true);

    foxdbg_server_thread = std::thread(foxdbg_server_thread_main);
    foxdbg_encoder_thread = std::thread(foxdbg_encoder_thread_main);

}

void foxdbg_threadpool_shutdown(void)
{

    running.store(false);

    if (foxdbg_server_thread.joinable())
    {
        foxdbg_server_thread.join();
    }

    if (foxdbg_encoder_thread.joinable())
    {
        foxdbg_encoder_thread.join();
    }

}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/


int foxdbg_server_thread_main()
{

    while (running.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        printf("Server thread running...\n");

        /* read from buffer */

        if (buffer.read_lock == false)
        {
            buffer.read_lock = true;

            uint64_t* data = (uint64_t*)buffer.data;
            printf("Data: %llu\n", *data);

            buffer.read_lock = false;
        }
    }

    printf("Server thread exiting...\n");

    return 0;
}

int foxdbg_encoder_thread_main()
{
    while (running.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        printf("Encoder thread running...\n");

        /* write to buffer */
        if (buffer.write_lock == false)
        {
            buffer.write_lock = true;

            *((uint64_t*)buffer.data) =  *((uint64_t*)buffer.data) + 1;

            buffer.write_lock = false;
        }
    }

    printf("Encoder thread exiting...\n");

    return 0;
}