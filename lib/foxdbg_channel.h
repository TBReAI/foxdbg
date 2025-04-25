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

#ifndef FOXDBG_CHANNEL_H
#define FOXDBG_CHANNEL_H



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

#define LARGE_BUFFER_SIZE (10*1024*1024) /* 1MB buffer */

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct 
{
    float x;
    float y;
    float z;
} foxdbg_vector3_t;

typedef struct 
{
    foxdbg_vector3_t position;
    foxdbg_vector3_t orientation;
} foxdbg_pose_t;

typedef enum
{
    FOXDBG_CHANNEL_TYPE_IMAGE,
    FOXDBG_CHANNEL_TYPE_POINTCLOUD,
    FOXDBG_CHANNEL_TYPE_CUBES,
    FOXDBG_CHANNEL_TYPE_LINES,
    FOXDBG_CHANNEL_TYPE_POSE,
    FOXDBG_CHANNEL_TYPE_FLOAT,
    FOXDBG_CHANNEL_TYPE_INTEGER,
    FOXDBG_CHANNEL_TYPE_BOOLEAN
} foxdbg_channel_type_t;

typedef struct
{
    int width;
    int height;
    int channels;
} foxdbg_image_info_t;

typedef struct foxdbg_channel_t
{
    const char *topic_name;
    uint64_t target_tx_time;
    uint64_t last_tx_time;

    int channel_id;
    int subscription_id;

    foxdbg_channel_type_t channel_type;

    foxdbg_buffer_t *data_buffer;
    foxdbg_buffer_t *info_buffer;

    struct foxdbg_channel_t *next;
} foxdbg_channel_t;

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