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

#include "foxdbg_channels.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define FOXDBG_PORT (8765U)
#define FOXDBG_VHOST ("localhost")

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

/* get the handle to a topic by ID */
int foxdbg_get_topic_id(const char* topic);

/*
**  TRANSMIT FUNCTIONS - THREAD SAFE
*/

void foxdbg_send_image_compressed(int topic, foxdbg_image_t image);

void foxdbg_send_pointcloud(int topic, foxdbg_pointcloud_t pointcloud);

/* send a set of cubes to the specified topic */
void foxdbg_send_cubes(const char* topic, const foxdbg_cube_t* cubes, size_t count);

void foxdbg_send_single(int topic, float value);

/*
** RECIEVE FUNCTIONS
*/

void foxdbg_set_rx_callback(int topic, foxdbg_rx_data_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_H */