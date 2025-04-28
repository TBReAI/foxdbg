/***************************************************************
**
** TBReAI Header File
**
** File         :  foxdbg_protocol.h
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server
**
***************************************************************/

#ifndef FOXDBG_PROTOCOL_HPP
#define FOXDBG_PROTOCOL_HPP

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "foxdbg_channel.h" 

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <libwebsockets.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define FOXDBG_DEBUG_PROTOCOL (0U)

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void foxdbg_protocol_init(lws_context *context, foxdbg_channel_t **channels, size_t *channel_count);

void foxdbg_protocol_shutdown(void);

void foxdbg_protocol_connect(lws *client);

void foxdbg_protocol_disconnect(lws *client);

void foxdbg_protocol_transmit_subscriptions(void);

void foxdbg_protocol_receive(char* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_PROTOCOL_HPP */