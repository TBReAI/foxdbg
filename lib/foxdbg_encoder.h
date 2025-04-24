/***************************************************************
**
** TBReAI Header File
**
** File         :  foxdbg_encoder.h
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-24 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server
**
***************************************************************/

#ifndef FOXDBG_ENCODER_H
#define FOXDBG_ENCODER_H

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
/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void foxdbg_encoder_init(void);

void foxdbg_encoder_shutdown(void);

void foxdbg_encode_channel(foxdbg_channel_t *channel);

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_ENCODER_H */