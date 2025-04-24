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
#include "foxdbg_atomic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

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

    //printf("Allocated buffer %p (%zu bytes)\n", buf->buffer_a, size);
    //printf("Allocated buffer %p (%zu bytes)\n", buf->buffer_b, size);

    buf->front_buffer = buf->buffer_a;
    buf->back_buffer = buf->buffer_b;
    buf->front_buffer_size = 0;
    buf->back_buffer_size = 0;

    buf->write_lock = 0;
    buf->read_lock = 0;
    buf->swap_lock = 0;

    *buffer = buf;

    return true;   
}

void foxdbg_buffer_begin_write(foxdbg_buffer_t* buffer, void **data, size_t *size)
{
    while (ATOMIC_READ_INT(&buffer->swap_lock))
    {
        /* wait for lock */
        YIELD_CPU();
    }

    ATOMIC_WRITE_INT(&buffer->write_lock, 1);

    *data = buffer->back_buffer;
    *size = buffer->buffer_size;

    //printf("BEGAN WRITE %p\n", buffer->back_buffer);

}

void foxdbg_buffer_end_write(foxdbg_buffer_t* buffer, size_t populated_size)
{
    buffer->back_buffer_size = populated_size;

    //printf("ENDED WRITE %p\n", buffer->back_buffer);

    ATOMIC_WRITE_INT(&buffer->write_lock, 0);

    swap_buffers(buffer);
}

void foxdbg_buffer_begin_read(foxdbg_buffer_t* buffer, void **data, size_t *size)
{

    while (ATOMIC_READ_INT(&buffer->swap_lock))
    {
        /* wait for lock */
        YIELD_CPU();
    }

    ATOMIC_WRITE_INT(&buffer->read_lock, 1);

    //printf("BEGAN READ %p\n", buffer->front_buffer);

    *data = buffer->front_buffer;
    *size = buffer->front_buffer_size;

    //printf("Buffer %p front size: %zu\n", buffer->front_buffer, buffer->front_buffer_size);
}

void foxdbg_buffer_end_read(foxdbg_buffer_t* buffer)
{
    //printf("ENDED READ %p\n", buffer->front_buffer);

    ATOMIC_WRITE_INT(&buffer->read_lock, 0);
}


/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void swap_buffers(foxdbg_buffer_t* buffer)
{
    /* First check if we need to wait for readers or writers */
    while (ATOMIC_READ_INT(&buffer->read_lock) || ATOMIC_READ_INT(&buffer->write_lock)) 
    {
        /* wait for lock */
        YIELD_CPU();
    }

    /* Now set swap lock to prevent new reads/writes */
    ATOMIC_WRITE_INT(&buffer->swap_lock, 1);

    /* Double check locks again (since there was a small window) */
    while (ATOMIC_READ_INT(&buffer->read_lock) || ATOMIC_READ_INT(&buffer->write_lock)) 
    {
        /* wait again to be safe */
        YIELD_CPU();
    }

    /* Now safe to swap */
    void* tmp = buffer->front_buffer;
    buffer->front_buffer = buffer->back_buffer;
    buffer->back_buffer = tmp;

    size_t tmp_size = buffer->front_buffer_size;
    buffer->front_buffer_size = buffer->back_buffer_size;
    buffer->back_buffer_size = tmp_size;

    //printf("SWAPPED %p <-> %p\n", buffer->front_buffer, buffer->back_buffer);

    ATOMIC_WRITE_INT(&buffer->swap_lock, 0);
}