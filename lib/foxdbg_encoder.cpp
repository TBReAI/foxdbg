/***************************************************************
**
** TBReAI Source File
**
** File         :  foxdbg_channels.c
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

#include "foxdbg_encoder.h"
#include "foxdbg_buffer.h"
#include "foxdbg_atomic.h"

#include <chrono>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <turbojpeg.h>
#include <libwebsockets.h>

#include <json/json.hpp>

using json = nlohmann::json;

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define TX_HEADER_SIZE (13U) 

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void encode_image(foxdbg_channel_t *channel);

static size_t encode_image_byte_array_to_buffer_optimized(
    uint8_t* tx_buffer, 
    size_t tx_buffer_size, 
    int width, int height, int components,
    const uint8_t* compressedImage, size_t compressedSize
);

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static uint8_t raw_data_buffer[10*1024*1024];
static uint8_t encoded_data_buffer[10*1024*1024];

static uint8_t info_data_buffer[1024*1024];

static tjhandle jpeg_handle = NULL;

static int jpegSubsamp = TJSAMP_420; /* Default to 4:2:0 subsampling */
static int jpegQuality = 25; /* Default quality factor */

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void foxdbg_encoder_init(void)
{
    jpeg_handle = tjInitCompress();
    if (jpeg_handle == NULL)
    {
        fprintf(stderr, "Failed to initialize JPEG compressor: %s\n", tjGetErrorStr());
    }
}

void foxdbg_encoder_shutdown(void)
{
    /* nothing to do */
}

void foxdbg_encode_channel(foxdbg_channel_t *channel)
{
    switch (channel->channel_type)
    {
        case FOXDBG_CHANNEL_TYPE_IMAGE:
        {
            encode_image(channel);
        } break;

        default:
        {
            /* do nothing */
        } return;
    }
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void encode_image(foxdbg_channel_t *channel)
{
    //printf("Encoding...\n");
    void *data;
    size_t data_size;
    foxdbg_buffer_begin_read(channel->raw_buffer, &data, &data_size);

    //printf("Data: %p, size: %llu\n", data, data_size);

    if (data_size <= sizeof(raw_data_buffer))
    {
        //printf("Copying %llu bytes from %p to %p\n", data_size, data, raw_data_buffer);
        memcpy(raw_data_buffer, data, data_size); /* crash here*/
        //printf("Copying done\n");
        foxdbg_buffer_end_read(channel->raw_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->raw_buffer);
        return;
    }

    //printf("Got to here...\n");


    foxdbg_buffer_begin_read(channel->info_buffer, &data, &data_size);

    if (data_size <= sizeof(info_data_buffer))
    {
        memcpy(info_data_buffer, data, data_size);
        foxdbg_buffer_end_read(channel->info_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->info_buffer);
        return;
    }

    /*printf("Encoding image: %d x %d, channels: %d\n", 
        ((foxdbg_image_info_t*)info_data_buffer)->width,
        ((foxdbg_image_info_t*)info_data_buffer)->height,
        ((foxdbg_image_info_t*)info_data_buffer)->channels
    );*/

    int pixelFormat = TJPF_RGB;

    if (((foxdbg_image_info_t*)info_data_buffer)->channels == 1)
    {
        pixelFormat = TJPF_GRAY;
    }
    else if (((foxdbg_image_info_t*)info_data_buffer)->channels == 3)
    {
        pixelFormat = TJPF_RGB;
    }
    else if (((foxdbg_image_info_t*)info_data_buffer)->channels == 4)
    {
        pixelFormat = TJPF_RGBA;
    }

    unsigned long compressedSize = 0;
    unsigned char* compressedImage = NULL;

    int result = tjCompress2(
        jpeg_handle,
        raw_data_buffer,
        ((foxdbg_image_info_t*)info_data_buffer)->width,
        0, // Pitch
        ((foxdbg_image_info_t*)info_data_buffer)->height,
        pixelFormat,
        &compressedImage,
        &compressedSize,
        jpegSubsamp,
        jpegQuality,
        TJFLAG_FASTDCT
    );

    if (result != 0)
    {
        fprintf(stderr, "Failed to compress image: %s\n", tjGetErrorStr());
        return;
    }

    size_t tx_buffer_size = sizeof(encoded_data_buffer);

    size_t bytes_written = encode_image_byte_array_to_buffer_optimized(
        encoded_data_buffer, 
        tx_buffer_size, 
        ((foxdbg_image_info_t*)info_data_buffer)->width,
        ((foxdbg_image_info_t*)info_data_buffer)->height,
        ((foxdbg_image_info_t*)info_data_buffer)->channels,
        compressedImage, 
        compressedSize
    );

    foxdbg_buffer_begin_write(channel->encoded_buffer, &data, &data_size);

    if (data_size >= (bytes_written + LWS_PRE + 13))
    {
        memcpy((uint8_t*)data + LWS_PRE + 13, encoded_data_buffer, bytes_written);

        foxdbg_buffer_end_write(channel->encoded_buffer, bytes_written + LWS_PRE + 13);
    }
    else
    {
        foxdbg_buffer_end_write(channel->encoded_buffer, 0);
        fprintf(stderr, "Encoded data buffer too small\n");
    }

    tjFree(compressedImage);
}


static size_t encode_image_byte_array_to_buffer_optimized(uint8_t* tx_buffer, size_t tx_buffer_size, int width, int height, int components, const uint8_t* compressedImage, size_t compressedSize) {

    // Estimate required buffer size (same as before)
    size_t estimated_json_overhead = 100; // A safe estimate
    size_t estimated_data_size = compressedSize * 4; // Max 3 digits + comma per byte
    size_t estimated_total_size = estimated_json_overhead + estimated_data_size + 10;

    if (estimated_total_size > tx_buffer_size) {
        fprintf(stderr, "Estimated JSON message too large for buffer\n");
        return 0; // Indicate failure
    }

    char* buffer_ptr = (char*)tx_buffer;
    size_t bytes_written = 0;

    // Write the fixed JSON parts
    bytes_written += sprintf(buffer_ptr + bytes_written, "{\"width\":%d,", width);
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"height\":%d,", height);
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"channels\":%d,", components);
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"encoding\":\"jpeg\",");
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"data\":[");

    // Write the byte array with manual conversion and fewer calls
    for (size_t i = 0; i < compressedSize; ++i) {
        uint8_t byte = compressedImage[i];
        if (byte >= 100) {
            buffer_ptr[bytes_written++] = '0' + (byte / 100);
            buffer_ptr[bytes_written++] = '0' + ((byte / 10) % 10);
            buffer_ptr[bytes_written++] = '0' + (byte % 10);
        } else if (byte >= 10) {
            buffer_ptr[bytes_written++] = '0' + (byte / 10);
            buffer_ptr[bytes_written++] = '0' + (byte % 10);
        } else {
            buffer_ptr[bytes_written++] = '0' + byte;
        }

        if (i < compressedSize - 1) {
            buffer_ptr[bytes_written++] = ',';
        }
    }

    // Close the data array and the JSON object
    buffer_ptr[bytes_written++] = ']';
    buffer_ptr[bytes_written++] = '}';
    buffer_ptr[bytes_written] = '\0'; // Null terminate

    return bytes_written; // Return the actual size of the JSON data
}
