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

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define LARGE_BUFFER_SIZE (1024*1024) /* 1MB buffer */

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

typedef struct
{
    float x;
    float y;
    float z;
    float roll;
    float pitch;
    float yaw;
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
    uint8_t buffer[LARGE_BUFFER_SIZE]; /* 1MB buffer */
} foxdbg_large_channel_buffer_t;

typedef struct
{
    union
    {
        foxdbg_pose_t pose_value;
        float float_value;
        int32_t int_value;
        bool bool_value;
    } data;
} foxdbg_small_channel_buffer_t;

typedef struct
{
    void* raw_buffer_a;
    void* raw_buffer_b;

    void* encoded_buffer_a;
    void* encoded_buffer_b;

    void* raw_front;
    void* raw_back;
    size_t raw_size; 

    void* encoded_front;
    void* encoded_back;
    size_t encoded_size;

    volatile bool encoded_swap_ready; 
    volatile size_t encoded_swap_complete; 
    
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