/***************************************************************
**
** TBReAI Source File
**
** File         :  foxdbg_buffer.c
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server
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

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#if defined(_MSC_VER)
    #include <windows.h>
    #define YIELD_CPU() Sleep(0)
#elif defined(__GNUC__) || defined(__clang__)
    #include <sched.h>
    #define YIELD_CPU() sched_yield()
#else
    #error "Unsupported compiler - implement atomic operations for your compiler"
#endif

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static void swap_buffers(foxdbg_buffer_t* buffer);

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

    buf->write_lock = false;
    buf->read_lock = false;
    buf->swap_lock = false;

    *buffer = buf;

    return true;   
}

void foxdbg_buffer_begin_write(foxdbg_buffer_t* buffer, void **data, size_t *size)
{
    while (buffer->swap_lock)
    {
        /* wait for lock */
        YIELD_CPU();
    }

    buffer->write_lock = true;

    *data = buffer->back_buffer;
    *size = buffer->buffer_size;
}

void foxdbg_buffer_end_write(foxdbg_buffer_t* buffer)
{
    buffer->write_lock = false;

    swap_buffers(buffer);
}

void foxdbg_buffer_begin_read(foxdbg_buffer_t* buffer, void **data, size_t *size)
{
    while (buffer->swap_lock)
    {
        /* wait for lock */
        YIELD_CPU();
    }

    buffer->read_lock = true;

    *data = buffer->front_buffer;
    *size = buffer->buffer_size;

}

void foxdbg_buffer_end_read(foxdbg_buffer_t* buffer)
{
    buffer->read_lock = false;
}


/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void swap_buffers(foxdbg_buffer_t* buffer)
{
    while (buffer->read_lock || buffer->write_lock) {
        /* wait for lock */
        YIELD_CPU();
    }

    buffer->swap_lock = true;

    void* tmp = buffer->front_buffer;
    buffer->front_buffer = buffer->back_buffer;
    buffer->back_buffer = tmp;

    buffer->swap_lock = false;
}