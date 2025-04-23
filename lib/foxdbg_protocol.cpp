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

#include "foxdbg.h"
#include "foxdbg_channels.h"
#include "foxdbg_protocol.h"

#include <sstream>
#include <chrono>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

static void send_json(json data);
static void send_data(uint32_t subscription_id, json *data);

static void send_server_info(void);
static void send_advertise(void);

static void receive_data(char* data, size_t len);
static void receive_advertise(json data);
static void receive_unadvertise(json data);

static json encode_image(foxdbg_channel_t *channel, foxdbg_image_t image);


/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static uint8_t tx_buffer[10*1024*1024]; /* 10MB tx buffer */

static foxdbg_channel_t *tx_channels;
static size_t tx_channel_count = 0;

static foxdbg_channel_t *rx_channels = NULL;
static size_t rx_channel_count = 0;

static struct lws_context *context = NULL;
static struct lws *client = NULL;

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void foxdbg_protocol_init(lws_context *init_context)
{

    tx_channels = foxdbg_get_tx_channels();
    tx_channel_count = foxdbg_get_tx_channels_count();

    rx_channels = foxdbg_get_rx_channels();
    rx_channel_count = foxdbg_get_rx_channels_count();

    context = init_context;
    client = NULL;
}

void foxdbg_protocol_connect(lws *new_client)
{

    if (client)
    {
        foxdbg_protocol_disconnect(client);
    }

    client = new_client;

    send_server_info();
    send_advertise();
}

void foxdbg_protocol_disconnect(lws *client)
{
    /* clear tx subscriptions */
    for (size_t i = 0; i < tx_channel_count; i++)
    {
        tx_channels[i].channel_data.subscription_id = -1;
    }
}

void foxdbg_protocol_send_data(foxdbg_channel_t *channel)
{

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
        receive_data(data, len);
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
        printf("FOXDBG: RX subscribe\n");
        if (json_object.contains("subscriptions"))
        {
            for (auto& subscription : json_object["subscriptions"])
            {
                try {

                    int subscription_id = subscription["id"].get<int>();
                    int channel_id = subscription["channelId"].get<int>();

                    for (int i = 0; i < tx_channel_count; i++)
                    {
                        if (i == channel_id)
                        {
                            tx_channels[i].channel_data.subscription_id = subscription_id;
                            printf("FOXDBG: RX channel %s id %d\n", tx_channels[i].topic, tx_channels[i].channel_data.subscription_id);
                        }
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
        printf("FOXDBG: RX unsubscribe\n");

        if (json_object.contains("subscriptionIds"))
        {
            for (auto& subscription_id : json_object["subscriptionIds"])
            {
                try {

                    int subscription_id_int = subscription_id.get<int>();
                    
                    for (int i = 0; i < tx_channel_count; i++)
                    {
                        if (tx_channels[i].channel_data.subscription_id == subscription_id_int)
                        {
                            tx_channels[i].channel_data.subscription_id = -1;
                            printf("FOXDBG: RX channel %s id %d\n", tx_channels[i].topic, tx_channels[i].channel_data.subscription_id);
                        }
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
        printf("FOXDBG: RX advertise\n");
        receive_advertise(json_object);
    }
    else if (json_object["op"] == "unadvertise")
    {
        printf("FOXDBG: RX unadvertise\n");
        receive_unadvertise(json_object);
    }
    else
    {
        printf("FOXDBG: RX %s\n", json_object.dump().c_str());
    }

}

void foxdbg_procotol_send_pending()
{

    for (size_t i = 0; i < tx_channel_count; i++)
    {
        if (tx_channels[i].channel_data.subscription_id != -1)
        {
            if (foxdbg_flag_get(&tx_channels[i].channel_data.unread_flag) &&
                !foxdbg_flag_get(&tx_channels[i].channel_data.write_flag))
            {
                
                foxdbg_flag_set(&tx_channels[i].channel_data.read_flag);
                
                json json_data = json::object();

                switch (tx_channels[i].channel_type)
                {
                    case FOXDBG_CHANNEL_TYPE_IMAGE:
                    {
                        json_data = encode_image(&tx_channels[i], tx_channels[i].data.image);
                    } break;

                    case FOXDBG_CHANNEL_TYPE_POINTCLOUD:
                    {
                        /* encode data */
                    } break;

                    case FOXDBG_CHANNEL_TYPE_CUBES:
                    {
                        /* encode data */
                    } break;

                    case FOXDBG_CHANNEL_TYPE_LINES:
                    {
                        /* encode data */
                    } break;

                    case FOXDBG_CHANNEL_TYPE_POSE:
                    {
                        /* encode data */
                    } break;

                    case FOXDBG_CHANNEL_TYPE_FLOAT:
                    {
                       /* encode data */
                    } break;

                    case FOXDBG_CHANNEL_TYPE_INTEGER:
                    {
                        /* encode data */
                    } break;

                    case FOXDBG_CHANNEL_TYPE_BOOLEAN:
                    {
                        /* encode data */
                    } break;

                }

                foxdbg_flag_clear(&tx_channels[i].channel_data.unread_flag);
                foxdbg_flag_clear(&tx_channels[i].channel_data.read_flag);


                send_data(tx_channels[i].channel_data.subscription_id, &json_data);
            }
        }
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

    printf("Sent JSON: %s\n", json_str.c_str());

}



static void send_data(uint32_t subscription_id, json *data)
{
    if (!client)
    {
        fprintf(stderr, "Client not connected\n");
        return;
    }

    char* out_ptr = reinterpret_cast<char*>(tx_buffer + LWS_PRE + TX_HEADER_SIZE);
    size_t max_json = sizeof(tx_buffer) - (LWS_PRE + TX_HEADER_SIZE);

    std::string json_str = data->dump(-1, ' ', false);
    size_t json_len = json_str.size();
    if (json_len >= max_json)
    {
        fprintf(stderr, "Data message too large\n");
        return;
    }
    memcpy(out_ptr, json_str.data(), json_len);

    // Header setup
    uint8_t* buf = tx_buffer + LWS_PRE;
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

    lws_write(client, tx_buffer + LWS_PRE, json_len + TX_HEADER_SIZE, LWS_WRITE_BINARY);

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

    for (size_t i = 0; i < tx_channel_count; i++)
    {

        std::string channel_schema;

        switch (tx_channels[i].channel_type)
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
            {"id", i},
            {"topic", tx_channels[i].topic},
            {"encoding", "json"},
            {"schemaName", channel_schema},
            {"schema", json::string_t()}
        };

        switch (tx_channels[i].channel_type)
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
    }

    send_json(channels_info);
}

static void receive_data(char* data, size_t len)
{

    if (len < 7)
    {
        fprintf(stderr, "Invalid data length\n");
        return;
    }
    
    uint32_t subscription_id = (data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24));

    json json_object = json::parse((char *)data + 5, (char *)data + len);
    
    if (!json_object.is_object() || !json_object.contains("value"))
    {
        fprintf(stderr, "Invalid JSON message\n");
        return;
    }

    for (int i = 0; i < rx_channel_count; i++)
    {
        if (rx_channels[i].channel_data.subscription_id == subscription_id)
        {

            std::string json_string = json_object.dump();
            printf("RX DATA FOR %s: %s\n", rx_channels[i].topic, json_string.c_str());

            switch (rx_channels[i].channel_type)
            {
                case FOXDBG_CHANNEL_TYPE_FLOAT:
                {
                    try
                    {
                        float value = json_object["value"].get<float>();

                        if (!foxdbg_flag_get(&rx_channels[i].channel_data.read_flag))
                        {
                            foxdbg_flag_set(&rx_channels[i].channel_data.write_flag);
                            
                            rx_channels[i].data.single.value = value;

                            foxdbg_flag_set(&rx_channels[i].channel_data.unread_flag);
                            foxdbg_flag_clear(&rx_channels[i].channel_data.write_flag);
                        }
                        
                    }
                    catch(...)
                    {
                        /* invalid data*/
                    }     
                } break;

                case FOXDBG_CHANNEL_TYPE_INTEGER:
                {
                    try
                    {
                        uint32_t value = json_object["value"].get<int>();
                        
                        if (!foxdbg_flag_get(&rx_channels[i].channel_data.read_flag))
                        {
                            foxdbg_flag_set(&rx_channels[i].channel_data.write_flag);
                            
                            rx_channels[i].data.integer.value = value;

                            foxdbg_flag_set(&rx_channels[i].channel_data.unread_flag);
                            foxdbg_flag_clear(&rx_channels[i].channel_data.write_flag);
                        }
                    }
                    catch(...)
                    {
                        /* invalid data*/
                    }                   
                } break;

                case FOXDBG_CHANNEL_TYPE_BOOLEAN:
                {
                    try
                    {
                        bool value = json_object["value"].get<bool>();

                        if (!foxdbg_flag_get(&rx_channels[i].channel_data.read_flag))
                        {
                            foxdbg_flag_set(&rx_channels[i].channel_data.write_flag);
                            
                            rx_channels[i].data.boolean.value = value;

                            foxdbg_flag_set(&rx_channels[i].channel_data.unread_flag);
                            foxdbg_flag_clear(&rx_channels[i].channel_data.write_flag);
                        }

                    }
                    catch(...)
                    {
                        /* invalid data*/
                    }     
                } break;

                default:
                {
                    /* not implemented */
                } break;
            }
        }
    }

}

static void receive_advertise(json data)
{
    if (data.contains("channels"))
    {
        for (auto& channel : data["channels"])
        {
            if (channel.contains("id") && channel.contains("topic") && channel.contains("encoding"))
            {
                for (int i = 0; i < rx_channel_count; i++)
                {
                    if (strcmp(rx_channels[i].topic, channel["topic"].get<std::string>().c_str()) == 0)
                    {
                        rx_channels[i].channel_data.subscription_id = channel["id"];
                        printf("FOXDBG: RX channel %s id %d\n", rx_channels[i].topic, rx_channels[i].channel_data.subscription_id);
                    }
                }
            }
        }
    }
}

static void receive_unadvertise(json data)
{
    if (data.contains("channelIds"))
    {
        for (auto& channelId : data["channelIds"])
        {
            for (int i = 0; i < rx_channel_count; i++)
            {
                if (rx_channels[i].channel_data.subscription_id == channelId.get<int>())
                {
                    rx_channels[i].channel_data.subscription_id = -1;
                    printf("FOXDBG: RX unadvertise channel %s id %d\n", rx_channels[i].topic, channelId.get<int>());
                }
            }
        }
    }
}

static json encode_image(foxdbg_channel_t *channel, foxdbg_image_t image)
{
    json image_data = {
        {"width", image.width},
        {"height", image.height},
        {"channels", image.channels},
        {"encoding", "jpeg"},
        {"data", json::array()}
    };

    /* build vector from raw buffer */
    std::vector<uint8_t> blob(
        image.buffer,
        image.buffer + image.image_size
    );

    image_data["data"] = std::move(blob);
    return image_data;
}