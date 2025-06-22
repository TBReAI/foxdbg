/***************************************************************
**
** TBReAI Source File
**
** File         :   foxdbg_buffer.c
** Module       :   foxdbg
** Author       :   SH
** Created      :   2025-04-14 (YYYY-MM-DD)
** License      :   MIT
** Description  :   Foxglove Debug Server
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "foxdbg_buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h> // For sched_yield
#endif

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

// Define YIELD_CPU for cross-platform compatibility
#ifdef _WIN32
#define YIELD_CPU() Sleep(0) // Yields the current time slice on Windows
#else
#define YIELD_CPU() sched_yield() // Yields the CPU on POSIX systems
#endif

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void swap_buffers(foxdbg_buffer_t* buffer);

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

bool foxdbg_buffer_alloc(size_t size, foxdbg_buffer_t **buffer)
{
    if (size == 0)
    {
        return false;
    }

    foxdbg_buffer_t* buf = (foxdbg_buffer_t*)malloc(sizeof(foxdbg_buffer_t));
    if (!buf)
    {
        return false;
    }

    buf->buffer_size = size;

    buf->buffer_a = malloc(size);
    buf->buffer_b = malloc(size);

    if (!buf->buffer_a || !buf->buffer_b)
    {
        free(buf->buffer_a);
        free(buf->buffer_b);
        free(buf);
        return false;
    }

    buf->front_buffer = buf->buffer_a;
    buf->back_buffer = buf->buffer_b;
    buf->front_buffer_size = 0;
    buf->back_buffer_size = 0;

#ifdef _WIN32
    buf->write_mutex = CreateMutex(NULL, FALSE, NULL);
    buf->read_mutex = CreateMutex(NULL, FALSE, NULL);
    buf->swap_mutex = CreateMutex(NULL, FALSE, NULL);

    if (!buf->write_mutex || !buf->read_mutex || !buf->swap_mutex)
    {
        if (buf->write_mutex) CloseHandle(buf->write_mutex);
        if (buf->read_mutex) CloseHandle(buf->read_mutex);
        if (buf->swap_mutex) CloseHandle(buf->swap_mutex);
        free(buf->buffer_a);
        free(buf->buffer_b);
        free(buf);
        return false;
    }
#else
    if (pthread_mutex_init(&buf->write_mutex, NULL) != 0 ||
        pthread_mutex_init(&buf->read_mutex, NULL) != 0 ||
        pthread_mutex_init(&buf->swap_mutex, NULL) != 0)
    {
        pthread_mutex_destroy(&buf->write_mutex);
        pthread_mutex_destroy(&buf->read_mutex);
        pthread_mutex_destroy(&buf->swap_mutex);
        free(buf->buffer_a);
        free(buf->buffer_b);
        free(buf);
        return false;
    }
#endif

    *buffer = buf;

    return true;
}

void foxdbg_buffer_free(foxdbg_buffer_t* buffer)
{
    if (!buffer) return;

    free(buffer->buffer_a);
    free(buffer->buffer_b);

#ifdef _WIN32
    CloseHandle(buffer->write_mutex);
    CloseHandle(buffer->read_mutex);
    CloseHandle(buffer->swap_mutex);
#else
    pthread_mutex_destroy(&buffer->write_mutex);
    pthread_mutex_destroy(&buffer->read_mutex);
    pthread_mutex_destroy(&buffer->swap_mutex);
#endif

    free(buffer);
}

void foxdbg_buffer_begin_write(foxdbg_buffer_t* buffer, void **data, size_t *size)
{
#ifdef _WIN32
    WaitForSingleObject(buffer->swap_mutex, INFINITE);
    ReleaseMutex(buffer->swap_mutex); // Release immediately, just to ensure no one else is in swap
    WaitForSingleObject(buffer->write_mutex, INFINITE);
#else
    pthread_mutex_lock(&buffer->swap_mutex);
    pthread_mutex_unlock(&buffer->swap_mutex); // Release immediately, just to ensure no one else is in swap
    pthread_mutex_lock(&buffer->write_mutex);
#endif

    *data = buffer->back_buffer;
    *size = buffer->buffer_size;
}

void foxdbg_buffer_end_write(foxdbg_buffer_t* buffer, size_t populated_size)
{
    buffer->back_buffer_size = populated_size;

#ifdef _WIN32
    ReleaseMutex(buffer->write_mutex);
#else
    pthread_mutex_unlock(&buffer->write_mutex);
#endif

    swap_buffers(buffer);
}

void foxdbg_buffer_begin_read(foxdbg_buffer_t* buffer, void **data, size_t *size)
{
#ifdef _WIN32
    WaitForSingleObject(buffer->swap_mutex, INFINITE);
    ReleaseMutex(buffer->swap_mutex); // Release immediately, just to ensure no one else is in swap
    WaitForSingleObject(buffer->read_mutex, INFINITE);
#else
    pthread_mutex_lock(&buffer->swap_mutex);
    pthread_mutex_unlock(&buffer->swap_mutex); // Release immediately, just to ensure no one else is in swap
    pthread_mutex_lock(&buffer->read_mutex);
#endif

    *data = buffer->front_buffer;
    *size = buffer->front_buffer_size;
}

void foxdbg_buffer_end_read(foxdbg_buffer_t* buffer)
{
#ifdef _WIN32
    ReleaseMutex(buffer->read_mutex);
#else
    pthread_mutex_unlock(&buffer->read_mutex);
#endif
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void swap_buffers(foxdbg_buffer_t* buffer)
{
#ifdef _WIN32
    // Acquire all locks
    WaitForSingleObject(buffer->swap_mutex, INFINITE);
    WaitForSingleObject(buffer->read_mutex, INFINITE);
    WaitForSingleObject(buffer->write_mutex, INFINITE);
#else
    // Acquire all locks
    pthread_mutex_lock(&buffer->swap_mutex);
    pthread_mutex_lock(&buffer->read_mutex);
    pthread_mutex_lock(&buffer->write_mutex);
#endif

    /* Now safe to swap */
    void* tmp = buffer->front_buffer;
    buffer->front_buffer = buffer->back_buffer;
    buffer->back_buffer = tmp;

    size_t tmp_size = buffer->front_buffer_size;
    buffer->front_buffer_size = buffer->back_buffer_size;
    buffer->back_buffer_size = tmp_size;

#ifdef _WIN32
    // Release all locks
    ReleaseMutex(buffer->write_mutex);
    ReleaseMutex(buffer->read_mutex);
    ReleaseMutex(buffer->swap_mutex);
#else
    // Release all locks
    pthread_mutex_unlock(&buffer->write_mutex);
    pthread_mutex_unlock(&buffer->read_mutex);
    pthread_mutex_unlock(&buffer->swap_mutex);
#endif
}