/***************************************************************
**
** TBReAI Header File
**
** File         :  foxdbg.h
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server
**
***************************************************************/

#ifndef FOXDBG_H
#define FOXDBG_H


/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define FOXDBG_PORT (8765U)
#define FOXDBG_VHOST ("0.0.0.0")

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/


/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


/* initialise the foxglove server */
void foxdbg_init(void);

/* poll for rx data callbacks */
void foxdbg_update(void);

/* shutdown the system */
void foxdbg_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_H */