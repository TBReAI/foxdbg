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
    void* data;
    size_t size;
    
    volatile bool write_lock;
    volatile bool read_lock;

} foxdbg_buffer_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_CHANNEL_H */