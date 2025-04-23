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
#include "foxdbg_channels.h"
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

foxdbg_channel_t* foxdbg_get_tx_channel_for_writing(int id, foxdbg_channel_type_t type);


/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/


static foxdbg_channel_t *tx_channels;
static size_t tx_channel_count = 0;

static foxdbg_channel_t *rx_channels;
static size_t rx_channel_count = 0;

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void foxdbg_init()
{

    tx_channels = foxdbg_get_tx_channels();
    tx_channel_count = foxdbg_get_tx_channels_count();

    rx_channels = foxdbg_get_rx_channels();
    rx_channel_count = foxdbg_get_rx_channels_count();
    
    foxdbg_thread_init();
}

void foxdbg_update(void)
{
    for (int i = 0; i < rx_channel_count; i++)
    {
        if (rx_channels[i].channel_data.rx_callback != NULL)
        {
            if (foxdbg_flag_get(&rx_channels[i].channel_data.unread_flag) && !foxdbg_flag_get(&rx_channels[i].channel_data.write_flag))
            {
                foxdbg_flag_set(&rx_channels[i].channel_data.read_flag);
                
                foxdbg_rx_data_t data;

                switch (rx_channels[i].channel_type)
                {
                    case FOXDBG_CHANNEL_TYPE_FLOAT:
                    {
                        data.single = rx_channels[i].data.single;
                    } break;

                    case FOXDBG_CHANNEL_TYPE_INTEGER:
                    {
                        data.integer = rx_channels[i].data.integer;
                    } break;

                    case FOXDBG_CHANNEL_TYPE_BOOLEAN:
                    {
                        data.boolean = rx_channels[i].data.boolean;
                    } break;

                    default:
                    {
                        /* invalid data type */
                    } break;
                }

                foxdbg_flag_clear(&rx_channels[i].channel_data.unread_flag);
                foxdbg_flag_clear(&rx_channels[i].channel_data.read_flag);

                rx_channels[i].channel_data.rx_callback(data);
            }
        }
    }
}

void foxdbg_shutdown(void)
{
    foxdbg_thread_shutdown();
    
}

int foxdbg_get_topic_id(const char* topic)
{
    for (size_t i = 0; i < tx_channel_count; i++)
    {
        if (strcmp(tx_channels[i].topic, topic) == 0)
        {
            return i;
        }
    }

    for (size_t i = 0; i < rx_channel_count; i++)
    {
        if (strcmp(rx_channels[i].topic, topic) == 0)
        {
            return i;
        }
    }

    return -1;
}


void foxdbg_send_image_compressed(int topic, foxdbg_image_t image)
{
    foxdbg_channel_t* channel = foxdbg_get_tx_channel_for_writing(topic, FOXDBG_CHANNEL_TYPE_IMAGE);
    if (channel == NULL)
    {
        return;
    }

    /* allocate and copy data */

    channel->data.image.width = image.width;
    channel->data.image.height = image.height;
    channel->data.image.channels = image.channels;
    channel->data.image.image_size = image.image_size;

    size_t target_size = image.buffer_size;

    if (channel->data.image.buffer == NULL || channel->data.image.buffer_size < target_size)
    {
        if (channel->data.image.buffer != NULL)
        {
            free(channel->data.image.buffer);
        }

        channel->data.image.buffer = (uint8_t*)malloc(target_size);
        if (channel->data.image.buffer == NULL)
        {
            foxdbg_flag_clear(&channel->channel_data.write_flag);
            return;
        }

        channel->data.image.buffer_size = target_size;
    }

    memcpy(channel->data.image.buffer, image.buffer, target_size);

    foxdbg_flag_set(&channel->channel_data.unread_flag);
    foxdbg_flag_clear(&channel->channel_data.write_flag);
}

void foxdbg_send_pointcloud(int topic, foxdbg_pointcloud_t pointcloud)
{
    
}

void foxdbg_send_single(int topic, float value)
{
    
}

void foxdbg_set_rx_callback(int topic, foxdbg_rx_data_callback_t callback)
{
    if (topic < 0 || topic >= rx_channel_count)
    {
        return;
    }

    rx_channels[topic].channel_data.rx_callback = callback;
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

foxdbg_channel_t* foxdbg_get_tx_channel_for_writing(int id, foxdbg_channel_type_t type)
{
    if (id < 0 || id >= tx_channel_count)
    {
        return NULL;
    }

    if (tx_channels[id].channel_type != type || 
        tx_channels[id].channel_data.subscription_id < 0)
    {
        return NULL;
    }


    if (foxdbg_flag_get(&tx_channels[id].channel_data.read_flag))
    {
        return NULL;
    }

    foxdbg_flag_set(&tx_channels[id].channel_data.write_flag);

    return &tx_channels[id];
}
