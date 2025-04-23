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

#include "foxdbg_channels.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <libwebsockets.h>

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


void foxdbg_protocol_init(lws_context *context);

void foxdbg_protocol_connect(lws *client);

void foxdbg_protocol_disconnect(lws *client);

//void foxdbg_protocol_send_data(foxdbg_channel_t *channel, json data);

void foxdbg_procotol_send_pending(void);

void foxdbg_protocol_receive(char* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_PROTOCOL_HPP */