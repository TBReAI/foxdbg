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
    size_t buffer_size;

    void* buffer_a;
    void* buffer_b;

    void* front_buffer;
    void* back_buffer;
    
    volatile bool write_lock;
    volatile bool read_lock;
    volatile bool swap_lock;

} foxdbg_buffer_t;


/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


bool foxdbg_buffer_alloc(size_t size, foxdbg_buffer_t **buffer);

void foxdbg_buffer_begin_write(foxdbg_buffer_t* buffer, void **data, size_t *size);
void foxdbg_buffer_end_write(foxdbg_buffer_t* buffer);

void foxdbg_buffer_begin_read(foxdbg_buffer_t* buffer, void **data, size_t *size);
void foxdbg_buffer_end_read(foxdbg_buffer_t* buffer);

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_CHANNEL_H */