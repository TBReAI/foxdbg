/***************************************************************
**
** TBReAI Source File
**
** File         :  foxdbg.c
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "foxdbg.h"
#include "foxdbg_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static foxdbg_channel_t *channels = NULL;
static size_t channel_count = 0;

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void foxdbg_init()
{
    channels = NULL;
    channel_count = 0;

    foxdbg_thread_init(&channels, &channel_count);
}


void foxdbg_shutdown(void)
{
    foxdbg_thread_shutdown();
}

int foxdbg_add_channel(const char *topic_name, foxdbg_channel_type_t channel_type, int target_hz)
{   
    size_t payload_size = 0;
    size_t info_size = 0;

    switch (channel_type)
    {
        case FOXDBG_CHANNEL_TYPE_IMAGE:
        {
            payload_size = LARGE_BUFFER_SIZE;
            info_size = sizeof(foxdbg_image_info_t);
        } break;

        case FOXDBG_CHANNEL_TYPE_POINTCLOUD:
        case FOXDBG_CHANNEL_TYPE_CUBES:
        case FOXDBG_CHANNEL_TYPE_LINES:
        {
            payload_size = LARGE_BUFFER_SIZE;
        } break;
        
        case FOXDBG_CHANNEL_TYPE_POSE:
        {
            payload_size = sizeof(foxdbg_pose_t);
        } break;

        case FOXDBG_CHANNEL_TYPE_FLOAT:
        {
            payload_size = sizeof(float);
        } break;
            break;
        case FOXDBG_CHANNEL_TYPE_INTEGER:
        {
            payload_size = sizeof(int);
        } break;
    
        case FOXDBG_CHANNEL_TYPE_BOOLEAN:
        {
            payload_size = sizeof(bool);
        } break;

        default:
        {
            return -1; /* Invalid channel type */
        } break;
    }

    foxdbg_buffer_t *data_buffer;
    if (!foxdbg_buffer_alloc(payload_size, &data_buffer))
    {
        return -1; /* Failed to allocate buffers */
    }

    foxdbg_buffer_t *info_buffer = NULL;
    if (info_size > 0 && !foxdbg_buffer_alloc(info_size, &info_buffer))
    {
        return -1; /* Failed to allocate info buffer */
    }
    

    foxdbg_channel_t *new_channel = malloc(sizeof(foxdbg_channel_t));
    if (!new_channel)
    {
        return -1; /* Failed to allocate channel */
    }

    new_channel->topic_name = topic_name;
    new_channel->target_tx_time = (1000 / target_hz);
    new_channel->last_tx_time = 0;

    printf("Adding channel %s with target tx time %llu\n", topic_name, new_channel->target_tx_time);

    new_channel->channel_type = channel_type;
    new_channel->data_buffer = data_buffer;
    new_channel->info_buffer = info_buffer;
    new_channel->subscription_id = -1;
    new_channel->channel_id = channel_count;
    new_channel->next = NULL;

    foxdbg_channel_t *current = channels;
    
    while (current && current->next)
    {
        current = current->next;
    }

    if (current)
    {
        current->next = new_channel;
    }
    else
    {
        channels = new_channel;
    }

    channel_count++;

    return new_channel->channel_id;
}

int foxdbg_get_channel(const char *topic_name)
{
    foxdbg_channel_t *current = channels;

    while (current)
    {
        if (strcmp(current->topic_name, topic_name) == 0)
        {
            return current->channel_id;
        }
        current = current->next;
    }

    return -1; /* Channel not found */
}

void foxdbg_write_channel(int channel_id, const void *data, size_t size)
{
    foxdbg_channel_t *current = channels;

    while (current)
    {
        if (current->channel_id == channel_id)
        {
            void *buffer_data = NULL;
            size_t buffer_size = 0;

            foxdbg_buffer_begin_write(current->data_buffer, &buffer_data, &buffer_size);

            //printf("Writing %zu bytes to %p\n", size, buffer_data);

            if (buffer_data && size <= buffer_size)
            {
                memcpy(buffer_data, data, size);
                foxdbg_buffer_end_write(current->data_buffer, size);
                return;
            }
            else
            {
                foxdbg_buffer_end_write(current->data_buffer, 0);
                return;
            }

        }
        current = current->next;
    }
}

void foxdbg_write_channel_info(int channel_id, const void *data, size_t size)
{
    foxdbg_channel_t *current = channels;

    while (current)
    {
        if (current->channel_id == channel_id && current->info_buffer)
        {
            void *buffer_data = NULL;
            size_t buffer_size = 0;

            foxdbg_buffer_begin_write(current->info_buffer, &buffer_data, &buffer_size);

            if (buffer_data && size <= buffer_size)
            {
                memcpy(buffer_data, data, size);
            }

            foxdbg_buffer_end_write(current->info_buffer, size);
            return;
        }
        current = current->next;
    }
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/
