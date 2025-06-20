/***************************************************************
**
** TBReAI Header File
**
** File         :  foxdbg_channel.h
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server
**
***************************************************************/

#ifndef FOXDBG_BUFFER_H
#define FOXDBG_BUFFER_H



/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/



#define LARGE_BUFFER_SIZE (1024*1024) /* 1MB buffer */

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct
{
    size_t buffer_size;         /* allocated size */

    void* buffer_a;
    void* buffer_b;

    void* front_buffer;
    size_t front_buffer_size;   /* populated size */
    void* back_buffer;
    size_t back_buffer_size;    /* populated size */
    
    volatile int write_lock;
    volatile int read_lock;
    volatile int swap_lock;

} foxdbg_buffer_t;


/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


bool foxdbg_buffer_alloc(size_t size, foxdbg_buffer_t **buffer);

void foxdbg_buffer_begin_write(foxdbg_buffer_t* buffer, void **data, size_t *size); /* size here is allocated size (i.e available for writing )*/
void foxdbg_buffer_end_write(foxdbg_buffer_t* buffer, size_t populated_size);

void foxdbg_buffer_begin_read(foxdbg_buffer_t* buffer, void **data, size_t *size); /* size here is populated size (i.e available for reading )*/
void foxdbg_buffer_end_read(foxdbg_buffer_t* buffer);

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_CHANNEL_H */