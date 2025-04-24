/***************************************************************
**
** TBReAI Header File
**
** File         :  foxdbg_thread.h
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server
**
***************************************************************/

#ifndef FOXDBG_THREAD_H
#define FOXDBG_THREAD_H

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "foxdbg_buffer.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


/* start FOXDBG thread pool */
void foxdbg_threadpool_init(foxdbg_channel_t **channels, size_t *channel_count);

/* stop FOXDBG thread pool */
void foxdbg_threadpool_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_THREAD_H */