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
#define _CRT_SECURE_NO_WARNINGS 

#include "foxdbg.h"
#include "foxdbg_protocol.h"
#include "foxdbg_atomic.h"

#include <sstream>
#include <chrono>
#include <vector>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <turbojpeg.h>
#include <libwebsockets.h>

#include <json/json.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

using json = nlohmann::json;

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#ifdef _WIN32
#include <windows.h>
uint64_t current_timestamp_ms() {
    LARGE_INTEGER frequency;
    LARGE_INTEGER performance_count;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&performance_count);
    long long milliseconds = (performance_count.QuadPart * 1000) / frequency.QuadPart;
    return milliseconds;
}
#else // Assume POSIX-like system
#include <sys/time.h>
uint64_t current_timestamp_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long milliseconds = (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
    return milliseconds;
}
#endif

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static void send_json(json data);
static void send_buffer(uint8_t *buffer, size_t buffer_size, size_t data_size, int subscription_id);

static void send_server_info(void);
static void send_advertise(void);

static void send_image(foxdbg_channel_t *channel);
static void send_pointcloud(foxdbg_channel_t *channel);
static void send_cubes(foxdbg_channel_t *channel);
static void send_lines(foxdbg_channel_t *channel);
static void send_pose(foxdbg_channel_t *channel);
static void send_transform(foxdbg_channel_t *channel);
static void send_location(foxdbg_channel_t *channel);
static void send_float(foxdbg_channel_t *channel);
static void send_integer(foxdbg_channel_t *channel);
static void send_bool(foxdbg_channel_t *channel);

static size_t encode_image_byte_array(
    uint8_t* tx_buffer, 
    size_t tx_buffer_size, 
    int width, int height, int components,
    const uint8_t* compressedImage, size_t compressedSize
);

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static uint8_t raw_data_buffer[10*1024*1024];
static uint8_t encode_buffer[10*1024*1024];
static uint8_t info_data_buffer[1024*1024];

static uint8_t tx_buffer[1024*1024]; /* 1MB tx buffer */

static size_t encode_buffer_size = sizeof(encode_buffer);
static size_t tx_buffer_size = sizeof(tx_buffer);

static struct lws_context *context = NULL;
static struct lws *client = NULL;

static foxdbg_channel_t **channels = NULL;
static size_t *channel_count = 0;

static tjhandle jpeg_handle = NULL;

static int jpegSubsamp = TJSAMP_420; /* Default to 4:2:0 subsampling */
static int jpegQuality = 25; /* Default quality factor */

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void foxdbg_protocol_init(lws_context *context_ptr, foxdbg_channel_t **channels_ptr, size_t *channel_count_ptr)
{
    context = context_ptr;
    channels = channels_ptr;
    channel_count = channel_count_ptr;

    jpeg_handle = tjInitCompress();
    if (jpeg_handle == NULL)
    {
        fprintf(stderr, "Failed to initialize JPEG compressor: %s\n", tjGetErrorStr());
    }
}

void foxdbg_protocol_shutdown(void)
{
    if (jpeg_handle)
    {
        tjDestroy(jpeg_handle);
        jpeg_handle = NULL;
    }

    context = NULL;
    channels = NULL;
    channel_count = 0;

    client = NULL;
}

void foxdbg_protocol_connect(lws *client_ptr)
{
    if (client)
    {
        foxdbg_protocol_disconnect(client);
    }

    client = client_ptr;

    send_server_info();
    send_advertise();
}

void foxdbg_protocol_disconnect(lws *client_ptr)
{
    client = NULL;

    foxdbg_channel_t *current = *channels;

    while (current)
    {
        ATOMIC_WRITE_INT(&current->subscription_id, -1);
        current = current->next;
    }
}

void foxdbg_protocol_receive(char* data, size_t len)
{
    if (len < 1)
    {
        fprintf(stderr, "Invalid data length\n");
        return;
    }

    if (data[0] == 0x01)
    {
        /* handle binary data */
        //receive_data(data, len);
        return;
    }

    json json_object = json::parse((char *)data, (char *)data + len);

    if (!json_object.is_object() || !json_object.contains("op"))
    {
        fprintf(stderr, "Invalid JSON message\n");
        return;
    }
    else if (json_object["op"] == "subscribe")
    {
        if (json_object.contains("subscriptions"))
        {
            for (auto& subscription : json_object["subscriptions"])
            {
                try {

                    int subscription_id = subscription["id"].get<int>();
                    int channel_id = subscription["channelId"].get<int>();

                    foxdbg_channel_t *channel = *channels;

                    while (channel)
                    {
                        if (channel->channel_id == channel_id)
                        {
                            ATOMIC_WRITE_INT(&channel->subscription_id, subscription_id);

                            #if FOXDBG_DEBUG_PROTOCOL
                                printf("FOXDBG: Client subscribed to %s\n", channel->topic_name);
                            #endif

                            return;
                        }
                        channel = channel->next;
                    }
                }
                catch (...)
                {
                    /* JSON error */
                }
            }
        }
    }
    else if (json_object["op"] == "unsubscribe")
    {
        if (json_object.contains("subscriptionIds"))
        {
            for (auto& subscription_id : json_object["subscriptionIds"])
            {
                try {

                    int subscription_id_int = subscription_id.get<int>();
                    
                    foxdbg_channel_t *channel = *channels;

                    while (channel)
                    {
                        if (ATOMIC_READ_INT(&channel->subscription_id) == subscription_id_int)
                        {
                            ATOMIC_WRITE_INT(&channel->subscription_id, -1);

                            #if FOXDBG_DEBUG_PROTOCOL
                                printf("FOXDBG: Client unsubscribed from %s\n", channel->topic_name);
                            #endif
                            return;
                        }
                        channel = channel->next;
                    }

                }
                catch (...)
                {
                    /* JSON error */
                }

                
            }
        }
    }
    else if (json_object["op"] == "advertise")
    {
        //printf("FOXDBG: RX advertise\n");
        //receive_advertise(json_object);
    }
    else if (json_object["op"] == "unadvertise")
    {
        //printf("FOXDBG: RX unadvertise\n");
        //receive_unadvertise(json_object);
    }
    else
    {
        //printf("FOXDBG: RX %s\n", json_object.dump().c_str());
    }

}

void foxdbg_protocol_transmit_subscriptions(void)
{
    foxdbg_channel_t *current = *channels;

    while (current)
    {   
        int subscription_id = ATOMIC_READ_INT(&current->subscription_id);

        uint64_t current_time = current_timestamp_ms();
        uint64_t last_time = current->last_tx_time;
        uint64_t elapsed = current_time - last_time;

        if (subscription_id >= 0 && elapsed > current->target_tx_time)
        {
            switch (current->channel_type)
            {
                case FOXDBG_CHANNEL_TYPE_IMAGE:
                {
                    send_image(current);
                } break;

                case FOXDBG_CHANNEL_TYPE_POINTCLOUD:
                {
                    send_pointcloud(current);
                } break;

                case FOXDBG_CHANNEL_TYPE_CUBES:
                {
                    send_cubes(current);
                } break;

                case FOXDBG_CHANNEL_TYPE_LINES:
                {
                    send_lines(current);
                } break;

                case FOXDBG_CHANNEL_TYPE_TRANSFORM:
                {
                    send_transform(current);
                } break;

                case FOXDBG_CHANNEL_TYPE_LOCATION:
                {
                    send_location(current);
                } break;

                case FOXDBG_CHANNEL_TYPE_POSE:
                {
                    send_pose(current);
                } break;

                case FOXDBG_CHANNEL_TYPE_FLOAT:
                {
                    send_float(current);
                } break;

                case FOXDBG_CHANNEL_TYPE_INTEGER:
                {
                    send_integer(current);
                } break;

                case FOXDBG_CHANNEL_TYPE_BOOLEAN:
                {
                    send_bool(current);
                } break;

                default:
                {

                } break;
            }
        }

        if (elapsed > current->target_tx_time)
        {
            current->last_tx_time = current_timestamp_ms();
        }

        current = current->next;
    }
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

static void send_json(json data)
{

    if (!client)
    {
        fprintf(stderr, "Client not connected\n");
        return;
    }

    std::string json_str = data.dump();
    size_t json_len = json_str.length();

    if (json_len > sizeof(tx_buffer) - LWS_PRE)
    {
        fprintf(stderr, "JSON message too large\n");
        return;
    }

    memcpy(tx_buffer + LWS_PRE, json_str.c_str(), json_len);

    lws_write(client, tx_buffer + LWS_PRE, json_len, LWS_WRITE_TEXT);

    #if FOXDBG_DEBUG_PROTOCOL
        printf("Sent JSON: %s\n", json_str.c_str());
    #endif

}

static void send_buffer(uint8_t *buffer, size_t buffer_size, size_t data_size, int subscription_id)
{
    if (!client)
    {
        fprintf(stderr, "Client not connected\n");
        return;
    }

    if ((data_size + LWS_PRE) > sizeof(tx_buffer))
    {
        fprintf(stderr, "Buffer message too large\n");
        return;
    }

    /* Header setup */
    uint8_t* buf = buffer;
    buf[0] = 0x01;
    buf[1] =  static_cast<uint8_t>( subscription_id       & 0xFF);
    buf[2] =  static_cast<uint8_t>((subscription_id >>  8) & 0xFF);
    buf[3] =  static_cast<uint8_t>((subscription_id >> 16) & 0xFF);
    buf[4] =  static_cast<uint8_t>((subscription_id >> 24) & 0xFF);

    uint64_t nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    for (int i = 0; i < 8; ++i)
    {
        buf[5 + i] = static_cast<uint8_t>((nsec >> (8 * i)) & 0xFF);
    }


    lws_write(client, buffer, data_size, LWS_WRITE_BINARY);

}


static void send_server_info(void)
{
    json server_info = {
        {"op", "serverInfo"},
        {"name", "TBReAI FOXDBG"},
        {"capabilities", {"clientPublish"}},
        {"supportedEncodings", {"json", "binary"}},
        {"metadata", json::object()}
    };

    send_json(server_info);
}

static void send_advertise(void)
{
    json channels_info = {
        {"op", "advertise"},
        {"channels", json::array()}
    };

    foxdbg_channel_t *current = *channels;

    while (current)
    {

        std::string channel_schema;

        switch (current->channel_type)
        {
            case FOXDBG_CHANNEL_TYPE_IMAGE:
            {
                channel_schema = "foxglove.CompressedImage";
            } break;

            case FOXDBG_CHANNEL_TYPE_POINTCLOUD:
            {
                channel_schema = "foxglove.PointCloud";
            } break;

            case FOXDBG_CHANNEL_TYPE_CUBES:
            {
                channel_schema = "foxglove.SceneUpdate";
            } break;

            case FOXDBG_CHANNEL_TYPE_LINES:
            {
                channel_schema = "foxglove.SceneUpdate";
            } break;

            case FOXDBG_CHANNEL_TYPE_POSE:
            {
                channel_schema = "foxglove.SceneUpdate";
            } break;

            case FOXDBG_CHANNEL_TYPE_TRANSFORM:
            {
                channel_schema = "foxglove.FrameTransform";
            } break;

            case FOXDBG_CHANNEL_TYPE_LOCATION:
            {
                channel_schema = "foxglove.LocationFix";
            } break;

            case FOXDBG_CHANNEL_TYPE_FLOAT:
            {
                channel_schema = "foxdbg.Float";
            } break;

            case FOXDBG_CHANNEL_TYPE_INTEGER:
            {
                channel_schema = "foxdbg.Integer";
            } break;

            case FOXDBG_CHANNEL_TYPE_BOOLEAN:
            {
                channel_schema = "foxdbg.Boolean";
            } break;

            default:
            {
                channel_schema = "foxglove.Unknown";
            } break;
        }

        json channel_info = {
            {"id", current->channel_id},
            {"topic", current->topic_name},
            {"encoding", "json"},
            {"schemaName", channel_schema},
            {"schema", json::string_t()}
        };

        switch (current->channel_type)
        {
            case FOXDBG_CHANNEL_TYPE_FLOAT:
            {
                json custom_schema = {
                    {"title", "foxdbg.Float"},
                    {"description", "float value"},
                    {"type", "object"},
                    {"properties", {
                        {"value", {
                            {"type", "number"},
                            {"description", "float value"}
                        }}
                    }}
                };
    
                channel_info["schema"] = custom_schema.dump();
            } break;

            case FOXDBG_CHANNEL_TYPE_INTEGER:
            {
                json custom_schema = {
                    {"title", "foxdbg.Integer"},
                    {"description", "float value"},
                    {"type", "object"},
                    {"properties", {
                        {"value", {
                            {"type", "integer"},
                            {"description", "int value"}
                        }}
                    }}
                };
    
                channel_info["schema"] = custom_schema.dump();
            } break;

            case FOXDBG_CHANNEL_TYPE_BOOLEAN:
            {
                json custom_schema = {
                    {"title", "foxdbg.Boolean"},
                    {"description", "bool value"},
                    {"type", "object"},
                    {"properties", {
                        {"value", {
                            {"type", "boolean"},
                            {"description", "bool value"}
                        }}
                    }}
                };
    
                channel_info["schema"] = custom_schema.dump();
            } break;


            default:
            {
                /* no custom schema */
            } break;
        }

        channels_info["channels"].push_back(channel_info);

        current = current->next;
    }

    send_json(channels_info);
}

static void send_image(foxdbg_channel_t *channel)
{
    void *data;
    size_t data_size;
    foxdbg_buffer_begin_read(channel->data_buffer, &data, &data_size);

    if (data_size <= sizeof(raw_data_buffer) && data_size > 0)
    {
        memcpy(raw_data_buffer, data, data_size);
        foxdbg_buffer_end_read(channel->data_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->data_buffer);
        return;
    }


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

    int subscription_id = ATOMIC_READ_INT(&channel->subscription_id);




    size_t bytes_written = encode_image_byte_array(
        encode_buffer, 
        encode_buffer_size, 
        ((foxdbg_image_info_t*)info_data_buffer)->width,
        ((foxdbg_image_info_t*)info_data_buffer)->height,
        ((foxdbg_image_info_t*)info_data_buffer)->channels,
        compressedImage, 
        compressedSize
    );


    if (tx_buffer_size >= (bytes_written + LWS_PRE + 13))
    {
        memcpy((uint8_t*)tx_buffer + LWS_PRE + 13, encode_buffer, bytes_written);


        send_buffer(
            (uint8_t*)tx_buffer + LWS_PRE, 
            tx_buffer_size, 
            bytes_written + 13,
            subscription_id
        );
    }


    tjFree(compressedImage);
}

static void send_pointcloud(foxdbg_channel_t *channel)
{
    void *data;
    size_t data_size;
    foxdbg_buffer_begin_read(channel->data_buffer, &data, &data_size);

    if (data_size <= sizeof(raw_data_buffer) && data_size > 0 && data_size % sizeof(foxdbg_vector4_t) == 0)
    {
        memcpy(raw_data_buffer, data, data_size);
        foxdbg_buffer_end_read(channel->data_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->data_buffer);
        return;
    }

    int subscription_id = ATOMIC_READ_INT(&channel->subscription_id);

    
    json j;
    j["timestamp"]["sec"] = 0;
    j["timestamp"]["nsec"] = 0;
    
    j["frame_id"] = "world";
    
    j["pose"]["position"] = {
        {"x", 0.0},
        {"y", 0.0},
        {"z", 0.6}
    };
    
    j["pose"]["orientation"] = {
        {"x", 0.0},
        {"y", 0.0},
        {"z", 0.0},
        {"w", 1.0}
    };
    
    j["point_stride"] = sizeof(foxdbg_vector4_t);
    
    j["fields"] = {
        {{"name", "x"}, {"offset", 0}, {"type", 7}},
        {{"name", "y"}, {"offset", 4}, {"type", 7}},
        {{"name", "z"}, {"offset", 8}, {"type", 7}},
        {{"name", "intensity"}, {"offset", 12}, {"type", 7}}
    };
    
    // Insert raw byte data into JSON array
    j["data"] = std::vector<uint8_t>((uint8_t*)data, (uint8_t*)data + data_size);

    std::string jsonStr = j.dump();

    if (tx_buffer_size >= (jsonStr.size() + LWS_PRE + 13))
    {
        memcpy((uint8_t*)tx_buffer + LWS_PRE + 13, jsonStr.c_str(), jsonStr.size());

        send_buffer(
            (uint8_t*)tx_buffer + LWS_PRE, 
            tx_buffer_size, 
            jsonStr.size() + 13,
            subscription_id
        );
    }
}

static void send_cubes(foxdbg_channel_t *channel)
{
    void *data;
    size_t data_size;
    foxdbg_buffer_begin_read(channel->data_buffer, &data, &data_size);

    if (data_size <= sizeof(raw_data_buffer) && data_size > 0 && data_size % sizeof(foxdbg_cube_t) == 0)
    {
        memcpy(raw_data_buffer, data, data_size);
        foxdbg_buffer_end_read(channel->data_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->data_buffer);
        return;
    }

    int subscription_id = ATOMIC_READ_INT(&channel->subscription_id);

    json j;
    j["entities"] = json::array();

    json entity;
    entity["frame_id"] = "world";
    entity["id"] = channel->topic_name;
    entity["timestamp"] = {
        {"sec", 0},
        {"nsec", 0}
    };

    size_t numCubes = data_size / sizeof(foxdbg_cube_t);
    foxdbg_cube_t *cubes = (foxdbg_cube_t*)raw_data_buffer;

    for (size_t i = 0; i < numCubes; ++i)
    {

        foxdbg_cube_t cube = cubes[i];

        json cube_object;

        cube_object["pose"]["position"] = {
            {"x", cube.position.x},
            {"y", cube.position.y},
            {"z", cube.position.z}
        };

        float pitch = cube.orientation.x;
        float roll = cube.orientation.y;
        float yaw = cube.orientation.z + (float)M_PI/2.0f;  // <-- Adjust yaw by +90 degrees (important!)
        
        float cy = cos(yaw * 0.5f);
        float sy = sin(yaw * 0.5f);
        float cp = cos(pitch * 0.5f);
        float sp = sin(pitch * 0.5f);
        float cr = cos(roll * 0.5f);
        float sr = sin(roll * 0.5f);
        
        // Standard XYZ euler to quaternion (for Foxglove arrow)
        float qx = sr * cp * cy - cr * sp * sy;
        float qy = cr * sp * cy + sr * cp * sy;
        float qz = cr * cp * sy - sr * sp * cy;
        float qw = cr * cp * cy + sr * sp * sy;    

        cube_object["pose"]["orientation"] = {
            {"x", qx},
            {"y", qy},
            {"z", qz},
            {"w", qw}
        };
        
        cube_object["size"] = {
            {"x", cube.size.x},
            {"y", cube.size.y},
            {"z", cube.size.z}
        };

        // Color mapping
        foxdbg_color_t color = cube.color;
        cube_object["color"] = {
            {"r", color.r},
            {"g", color.g},
            {"b", color.b},
            {"a", color.a}
        };

        entity["cubes"].push_back(cube_object);
    }

    j["entities"].push_back(entity);

    std::string jsonStr = j.dump();

    if (tx_buffer_size >= (jsonStr.size() + LWS_PRE + 13))
    {
        memcpy((uint8_t*)tx_buffer + LWS_PRE + 13, jsonStr.c_str(), jsonStr.size());

        send_buffer(
            (uint8_t*)tx_buffer + LWS_PRE, 
            tx_buffer_size, 
            jsonStr.size() + 13,
            subscription_id
        );
    }
}

static void send_lines(foxdbg_channel_t *channel)
{
    void *data;
    size_t data_size;
    foxdbg_buffer_begin_read(channel->data_buffer, &data, &data_size);

    if (data_size <= sizeof(raw_data_buffer) && data_size > 0 && data_size % sizeof(foxdbg_line_t) == 0)
    {
        memcpy(raw_data_buffer, data, data_size);
        foxdbg_buffer_end_read(channel->data_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->data_buffer);
        return;
    }

    int subscription_id = ATOMIC_READ_INT(&channel->subscription_id);

    json j;
    j["entities"] = json::array();

    json entity;
    entity["frame_id"] = "world";
    entity["id"] = channel->topic_name;
    entity["timestamp"] = {
        {"sec", 0},
        {"nsec", 0}
    };

    size_t numLines = data_size / sizeof(foxdbg_line_t);
    foxdbg_line_t *lines = (foxdbg_line_t*)raw_data_buffer;

    for (size_t i = 0; i < numLines; ++i)
    {

        foxdbg_line_t line = lines[i];

        json line_object;

        line_object["type"] = 2; /* LINE_LIST */

        line_object["pose"]["position"] = {
            {"x", 0.0f}, {"y", 0.0f}, {"z", 0.0f}
        };

        line_object["pose"]["orientation"] = {
            {"x", 0.0f}, {"y", 0.0f}, {"z", 0.0f}, {"w", 1.0f}
        };

        line_object["thickness"] = line.thickness;
        line_object["scale_invariant"] = false;
        line_object["points"] = json::array();
        line_object["points"].push_back({
            {"x", line.start.x},
            {"y", line.start.y},
            {"z", line.start.z}
        });
        line_object["points"].push_back({
            {"x", line.end.x},
            {"y", line.end.y},
            {"z", line.end.z}
        });

        // Color mapping
        foxdbg_color_t color = line.color;
        line_object["color"] = {
            {"r", color.r},
            {"g", color.g},
            {"b", color.b},
            {"a", color.a}
        };

        entity["lines"].push_back(line_object);

    }

    j["entities"].push_back(entity);

    std::string jsonStr = j.dump();

    if (tx_buffer_size >= (jsonStr.size() + LWS_PRE + 13))
    {
        memcpy((uint8_t*)tx_buffer + LWS_PRE + 13, jsonStr.c_str(), jsonStr.size());

        send_buffer(
            (uint8_t*)tx_buffer + LWS_PRE, 
            tx_buffer_size, 
            jsonStr.size() + 13,
            subscription_id
        );
    }
}

static void send_pose(foxdbg_channel_t *channel)
{
    void *data;
    size_t data_size;
    foxdbg_buffer_begin_read(channel->data_buffer, &data, &data_size);

    if (data_size <= sizeof(raw_data_buffer) && data_size == sizeof(foxdbg_pose_t))
    {
        memcpy(raw_data_buffer, data, data_size);
        foxdbg_buffer_end_read(channel->data_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->data_buffer);
        return;
    }

    int subscription_id = ATOMIC_READ_INT(&channel->subscription_id);

    json j;
    j["entities"] = json::array();

    json entity;
    entity["frame_id"] = "world";
    entity["id"] = channel->topic_name;
    entity["timestamp"] = {
        {"sec", 0},
        {"nsec", 0}
    };

    entity["arrows"] = json::array();
    
    foxdbg_pose_t pose = *(foxdbg_pose_t*)raw_data_buffer;

    float pitch = pose.orientation.x;
    float roll = pose.orientation.y;
    float yaw = pose.orientation.z + (float)M_PI/2.0f;  // <-- Adjust yaw by +90 degrees (important!)
    
    float cy = cos(yaw * 0.5f);
    float sy = sin(yaw * 0.5f);
    float cp = cos(pitch * 0.5f);
    float sp = sin(pitch * 0.5f);
    float cr = cos(roll * 0.5f);
    float sr = sin(roll * 0.5f);
    
    // Standard XYZ euler to quaternion (for Foxglove arrow)
    float qx = sr * cp * cy - cr * sp * sy;
    float qy = cr * sp * cy + sr * cp * sy;
    float qz = cr * cp * sy - sr * sp * cy;
    float qw = cr * cp * cy + sr * sp * sy;    

    json arrow_object;
    arrow_object["pose"]["position"] = {
        {"x", pose.position.x},
        {"y", pose.position.y},
        {"z", pose.position.z}
    };

    arrow_object["pose"]["orientation"] = {
        {"x", qx},
        {"y", qy},
        {"z", qz},
        {"w", qw}
    };
    
    arrow_object["shaft_length"] = 0.5f;
    arrow_object["shaft_diameter"] = 0.05f;
    arrow_object["head_length"] = 0.15f;
    arrow_object["head_diameter"] = 0.1f;

    // Color mapping
    foxdbg_color_t color = pose.color;
    arrow_object["color"] = {
        {"r", color.r},
        {"g", color.g},
        {"b", color.b},
        {"a", color.a}
    };

    entity["arrows"].push_back(arrow_object);

    j["entities"].push_back(entity);

    std::string jsonStr = j.dump();

    if (tx_buffer_size >= (jsonStr.size() + LWS_PRE + 13))
    {
        memcpy((uint8_t*)tx_buffer + LWS_PRE + 13, jsonStr.c_str(), jsonStr.size());

        send_buffer(
            (uint8_t*)tx_buffer + LWS_PRE, 
            tx_buffer_size, 
            jsonStr.size() + 13,
            subscription_id
        );
    }
}

static void send_transform(foxdbg_channel_t *channel)
{

    float *data;
    size_t data_size;
    foxdbg_buffer_begin_read(channel->data_buffer, (void**)&data, &data_size);

    if (data_size <= sizeof(raw_data_buffer) && data_size == sizeof(foxdbg_transform_t))
    {
        memcpy(raw_data_buffer, data, data_size);
        foxdbg_buffer_end_read(channel->data_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->data_buffer);
        return;
    }

    int subscription_id = ATOMIC_READ_INT(&channel->subscription_id);


    foxdbg_transform_t *transform = (foxdbg_transform_t*)raw_data_buffer;

    json json_data;
    json_data["timestamp"]["sec"] = 0;
    json_data["timestamp"]["nsec"] = 0;

    json_data["parent_frame_id"] = transform->parent_id;
    json_data["child_frame_id"] = transform->id;

    json_data["translation"] = {
        {"x", transform->position.x},
        {"y", transform->position.y},
        {"z", transform->position.z}
    };

    float pitch = transform->orientation.x;
    float roll = transform->orientation.y;
    float yaw = transform->orientation.z;

    float cy = cos(yaw * 0.5f);
    float sy = sin(yaw * 0.5f);
    float cp = cos(pitch * 0.5f);
    float sp = sin(pitch * 0.5f);
    float cr = cos(roll * 0.5f);
    float sr = sin(roll * 0.5f);

    // Standard XYZ euler to quaternion (for Foxglove arrow)
    float qx = sr * cp * cy - cr * sp * sy;
    float qy = cr * sp * cy + sr * cp * sy;
    float qz = cr * cp * sy - sr * sp * cy;
    float qw = cr * cp * cy + sr * sp * sy;    

    json_data["rotation"] = {
        {"x", qx},
        {"y", qy},
        {"z", qz},
        {"w", qw}
    };

    std::string json_str = json_data.dump();
    size_t json_len = json_str.length();

    if (json_len + LWS_PRE + 13 < sizeof(tx_buffer))
    {
        memcpy(tx_buffer + LWS_PRE + 13, json_str.c_str(), json_len);

        send_buffer(
            (uint8_t*)tx_buffer + LWS_PRE, 
            tx_buffer_size, 
            json_len + 13,
            subscription_id
        );
    }
}

static void send_location(foxdbg_channel_t *channel)
{
    void *data;
    size_t data_size;
    foxdbg_buffer_begin_read(channel->data_buffer, &data, &data_size);

    if (data_size <= sizeof(raw_data_buffer) && data_size == sizeof(foxdbg_location_t))
    {
        memcpy(raw_data_buffer, data, data_size);
        foxdbg_buffer_end_read(channel->data_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->data_buffer);
        return;
    }

    int subscription_id = ATOMIC_READ_INT(&channel->subscription_id);

    foxdbg_location_t *location = (foxdbg_location_t*)raw_data_buffer;

    json json_data = {
        {"timestamp", {
            {"sec", location->timestamp_sec},
            {"nsec", location->timestamp_nsec}
        }},
        {"frame_id", "world"},
        {"latitude", location->latitude},
        {"longitude", location->longitude},
        {"altitude", location->altitude}
    };

    json_data["position_covariance"] = std::vector<double>(9, 0.0);
    json_data["position_covariance_type"] = 0;

    std::string json_str = json_data.dump();
    size_t json_len = json_str.length();

    if (json_len + LWS_PRE + 13 < sizeof(tx_buffer))
    {
        memcpy(tx_buffer + LWS_PRE + 13, json_str.c_str(), json_len);

        send_buffer(
            (uint8_t*)tx_buffer + LWS_PRE, 
            tx_buffer_size, 
            json_len + 13,
            subscription_id
        );
    }
}

static void send_float(foxdbg_channel_t *channel)
{

    void *data;
    size_t data_size;
    foxdbg_buffer_begin_read(channel->data_buffer, (void**)&data, &data_size);

    if (data_size <= sizeof(raw_data_buffer) && data_size == sizeof(float))
    {
        memcpy(raw_data_buffer, data, data_size);
        foxdbg_buffer_end_read(channel->data_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->data_buffer);
        return;
    }

    int subscription_id = ATOMIC_READ_INT(&channel->subscription_id);

    float *raw = (float*)raw_data_buffer;

    json json_data = {
        {"value", (*raw)}
    };

    std::string json_str = json_data.dump();
    size_t json_len = json_str.length();

    if (json_len + LWS_PRE + 13 < sizeof(tx_buffer))
    {
        memcpy(tx_buffer + LWS_PRE + 13, json_str.c_str(), json_len);

        send_buffer(
            (uint8_t*)tx_buffer + LWS_PRE, 
            tx_buffer_size, 
            json_len + 13,
            subscription_id
        );
    }
    
}

static void send_integer(foxdbg_channel_t *channel)
{
    void *data;
    size_t data_size;
    foxdbg_buffer_begin_read(channel->data_buffer, (void**)&data, &data_size);

    if (data_size <= sizeof(raw_data_buffer) && data_size == sizeof(int))
    {
        memcpy(raw_data_buffer, data, data_size);
        foxdbg_buffer_end_read(channel->data_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->data_buffer);
        return;
    }

    int subscription_id = ATOMIC_READ_INT(&channel->subscription_id);

    int *raw = (int*)raw_data_buffer;

    json json_data = {
        {"value", (*raw)}
    };

    std::string json_str = json_data.dump();
    size_t json_len = json_str.length();

    if (json_len + LWS_PRE + 13 < sizeof(tx_buffer))
    {
        memcpy(tx_buffer + LWS_PRE + 13, json_str.c_str(), json_len);

        send_buffer(
            (uint8_t*)tx_buffer + LWS_PRE, 
            tx_buffer_size, 
            json_len + 13,
            subscription_id
        );
    }
}


static void send_bool(foxdbg_channel_t *channel)
{
    void *data;
    size_t data_size;
    foxdbg_buffer_begin_read(channel->data_buffer, (void**)&data, &data_size);

    if (data_size <= sizeof(raw_data_buffer) && data_size == sizeof(bool))
    {
        memcpy(raw_data_buffer, data, data_size);
        foxdbg_buffer_end_read(channel->data_buffer);
    }
    else 
    {
        foxdbg_buffer_end_read(channel->data_buffer);
        return;
    }

    int subscription_id = ATOMIC_READ_INT(&channel->subscription_id);

    bool *raw = (bool*)raw_data_buffer;

    json json_data = {
        {"value", (*raw)}
    };

    std::string json_str = json_data.dump();
    size_t json_len = json_str.length();

    if (json_len + LWS_PRE + 13 < sizeof(tx_buffer))
    {
        memcpy(tx_buffer + LWS_PRE + 13, json_str.c_str(), json_len);

        send_buffer(
            (uint8_t*)tx_buffer + LWS_PRE, 
            tx_buffer_size, 
            json_len + 13,
            subscription_id
        );
    }
}


static size_t encode_image_byte_array(uint8_t* tx_buffer, size_t tx_buffer_size, int width, int height, int components, const uint8_t* compressedImage, size_t compressedSize) {

    /* Estimate required buffer size (same as before) */
    size_t estimated_json_overhead = 100; /* A safe estimate */
    size_t estimated_data_size = compressedSize * 4; /* Max 3 digits + comma per byte */
    size_t estimated_total_size = estimated_json_overhead + estimated_data_size + 10;

    if (estimated_total_size > tx_buffer_size) {
        fprintf(stderr, "Estimated JSON message too large for buffer\n");
        return 0; // Indicate failure
    }

    char* buffer_ptr = (char*)tx_buffer;
    size_t bytes_written = 0;

    /* Write the fixed JSON parts */
    bytes_written += sprintf(buffer_ptr + bytes_written, "{\"width\":%d,", width);
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"height\":%d,", height);
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"channels\":%d,", components);
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"encoding\":\"jpeg\",");
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"data\":[");

    /* Write the byte array with manual conversion and fewer calls */
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

    /* Close the data array and the JSON object */
    buffer_ptr[bytes_written++] = ']';
    buffer_ptr[bytes_written++] = '}';
    buffer_ptr[bytes_written] = '\0'; /* Null terminate */

    return bytes_written; /* Return the actual size of the JSON data */
}
