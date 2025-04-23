/***************************************************************
**
** TBReAI Header File
**
** File         :  foxdbg_channels.h
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server
**
***************************************************************/

#ifndef FOXDBG_CHANNELS_H
#define FOXDBG_CHANNELS_H


/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "foxdbg_concurrent.h"

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define FOXDBG_CHANNEL_EMPTY .channel_data = { .subscription_id = -1, .write_flag = 0, .read_flag = 0, .unread_flag = 0, .rx_callback = NULL }, .data.data = {0}

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/* DATA TYPES */

typedef struct 
{
    float x;
    float y;
    float z;
} foxdbg_point_t;

typedef struct 
{
    float pitch;
    float yaw;
    float roll;
} foxdbg_orientation_t;

typedef struct 
{
    float width;
    float height;
    float depth;
} foxdbg_size_t;

typedef struct 
{
    float red;
    float green;
    float blue;
} foxdbg_color_t;

typedef struct 
{
    foxdbg_point_t pos;
    foxdbg_size_t size;
    foxdbg_color_t color;
} foxdbg_cube_t;

typedef struct 
{
    foxdbg_point_t start;
    foxdbg_point_t end;
    foxdbg_color_t color;
} foxdbg_line_t;


/* CHANNEL DATA TYPES */

typedef struct 
{
    size_t width;
    size_t height;
    size_t channels;
    size_t image_size;
    uint8_t *buffer;
    size_t buffer_size;
} foxdbg_image_t;

typedef struct 
{
    size_t points;
    uint8_t *buffer;
    size_t buffer_size;
} foxdbg_pointcloud_t;

typedef struct 
{
    size_t count;
    uint8_t *buffer;
    size_t buffer_size;
} foxdbg_cubes_t;

typedef struct 
{
    size_t count;
    uint8_t *buffer;
    size_t buffer_size;
} foxdbg_lines_t;

typedef struct 
{
    foxdbg_point_t pos;
    foxdbg_orientation_t orientation;
    foxdbg_color_t color;
} foxdbg_pose_t;

typedef struct 
{
    float value;
} foxdbg_float_t;

typedef struct 
{
    uint32_t value;
} foxdbg_integer_t;

typedef struct 
{
    bool value;
} foxdbg_boolean_t;

/* CHANNEL DEFINITIONS */

typedef union
{
    foxdbg_float_t single;
    foxdbg_integer_t integer;
    foxdbg_boolean_t boolean;
} foxdbg_rx_data_t;

typedef void (*foxdbg_rx_data_callback_t)(foxdbg_rx_data_t data);


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
    int32_t subscription_id;
    foxdbg_flag_t write_flag;
    foxdbg_flag_t read_flag;
    foxdbg_flag_t unread_flag;
    foxdbg_rx_data_callback_t rx_callback;
} foxdbg_channel_data_t;

typedef struct 
{
    const char* topic;
    foxdbg_channel_type_t channel_type;

    foxdbg_channel_data_t channel_data;

    union
    {
        foxdbg_image_t image;
        foxdbg_pointcloud_t pointcloud;
        foxdbg_cubes_t cubes;
        foxdbg_lines_t lines;
        foxdbg_pose_t pose;
        foxdbg_float_t single;
        foxdbg_integer_t integer;
        foxdbg_boolean_t boolean;

        uint8_t data[64];
    } data;

} foxdbg_channel_t;

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

foxdbg_channel_t* foxdbg_get_tx_channels(void);
size_t foxdbg_get_tx_channels_count(void);

foxdbg_channel_t* foxdbg_get_rx_channels(void);
size_t foxdbg_get_rx_channels_count(void);

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_CHANNELS_H */