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
#include "foxdbg_buffer.h"

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

static foxdbg_buffer_t *buffer;

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void foxdbg_threadpool_init()
{   
    running.store(true);

    foxdbg_buffer_alloc(LARGE_BUFFER_SIZE, &buffer);

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

int foxdbg_server_thread_main() {
    while (running.load()) {

        // Check for incoming data
        void* data = nullptr;
        size_t size = 0;

        foxdbg_buffer_begin_read(buffer, &data, &size);

        if (data != nullptr) {
            uint64_t* data_ptr = (uint64_t*)data;
            printf("Received data: %llu\n", *data_ptr);
        }

        foxdbg_buffer_end_read(buffer);
    }

    printf("Server thread exiting...\n");
    return 0;
}

int foxdbg_encoder_thread_main() {

    while (running.load()) {

        static uint64_t counter = 0;
        counter++;

        void* data = nullptr;
        size_t size = 0;

        foxdbg_buffer_begin_write(buffer, &data, &size);

        if (data != nullptr) {
            uint64_t* data_ptr = (uint64_t*)data;
            *data_ptr = counter;
        }

        foxdbg_buffer_end_write(buffer);

    }

    printf("Encoder thread exiting...\n");
    return 0;
}