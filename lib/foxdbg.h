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

#include "foxdbg_channel.h"

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

/* create a new channel */
int foxdbg_add_channel(const char *topic_name, foxdbg_channel_type_t channel_type, int target_hz);

int foxdbg_get_channel(const char *topic_name);

int foxdbg_add_rx_channel(const char *topic_name, foxdbg_channel_type_t channel_type);

int foxdbg_get_rx_channel(const char *topic_name);

void foxdbg_write_channel(int channel_id, const void *data, size_t size);

void foxdbg_write_channel_info(int channel_id, const void *data, size_t size);


#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_H */