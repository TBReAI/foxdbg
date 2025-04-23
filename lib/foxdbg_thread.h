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


/* start FOXDBG thread */
void foxdbg_thread_init(void);

/* stop FOXDBG thread */
void foxdbg_thread_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_THREAD_H */