/***************************************************************
**
** TBReAI Source File
**
** File         :  foxdbg_thread.cpp
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server Thread
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include "foxdbg_atomic.h"

#include "foxdbg.h"
#include "foxdbg_threadpool.h"
#include "foxdbg_buffer.h"

#include "foxdbg_protocol.h"
#include "foxdbg_encoder.h"

#include <libwebsockets.h>
#include <stdio.h>

#include <chrono>
#include <thread>
#include <atomic>


/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static int foxdbg_server_thread_main();
static int foxdbg_encoder_thread_main();

static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);


/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/

static struct lws_protocols protocols[] = {
    {
        "foxglove.websocket.v1",    /* Protocol name */
        websocket_callback,         /* Callback function */
        0,                          /* Max frame size */
        1024*1024                   /* Buffer size */
    },
    { NULL, NULL, 0, 0 }            /* Terminator */
};

static struct lws_context *context = NULL;

static std::thread foxdbg_server_thread;
static std::thread foxdbg_encoder_thread;

static std::atomic_bool running(false);

static foxdbg_channel_t **channels = NULL;
static size_t *channel_count = NULL;

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void foxdbg_threadpool_init(foxdbg_channel_t **channels_ptr, size_t *channel_count_ptr)
{   
    running.store(true);

    channels = channels_ptr;
    channel_count = channel_count_ptr;

    foxdbg_server_thread = std::thread(foxdbg_server_thread_main);
    foxdbg_encoder_thread = std::thread(foxdbg_encoder_thread_main);
}

void foxdbg_threadpool_shutdown(void)
{
    lws_cancel_service(context);

    running.store(false);


    if (foxdbg_server_thread.joinable())
    {
        foxdbg_server_thread.join();
    }

    if (foxdbg_encoder_thread.joinable())
    {
        foxdbg_encoder_thread.join();
    }

}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

int foxdbg_server_thread_main() 
{

    struct lws_context_creation_info info = { 0 };

    info.port = FOXDBG_PORT;                /* Port to listen on */
    info.vhost_name = FOXDBG_VHOST;         /* Vhost name */
    info.protocols = protocols;             /* Set the protocols */
    info.gid = -1;                          /* Group ID */
    info.uid = -1;                          /* User ID */
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    context = lws_create_context(&info);
    if (context == NULL) 
    {
        fprintf(stderr, "libwebsockets init failed\n");
    }

    foxdbg_protocol_init(context, channels, channel_count);

    printf("FOXDBG: Server started on port %d\n", FOXDBG_PORT);

    while (running.load()) 
    {
        lws_service(context, 0);
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    printf("Server thread exiting...\n");
    return 0;
}

int foxdbg_encoder_thread_main() {

    foxdbg_encoder_init();

    foxdbg_channel_t *current = *channels;

    while (running.load()) 
    {
        
        if (current == NULL)
        {
            YIELD_CPU();
            current = *channels;
        }
        else
        {
            if (ATOMIC_READ_INT(&current->subscription_id) >= 0)
            {
                foxdbg_encode_channel(current);
            }

            current = current->next;
        }

        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    foxdbg_encoder_shutdown();

    printf("Encoder thread exiting...\n");
    return 0;
}

static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    
    if (!running.load()) 
    {
        printf("FOXDBG: Server not running, closing connection\n");
        return -1;
    }

    switch (reason) 
    {
        case LWS_CALLBACK_ESTABLISHED:
        {
            printf("FOXDBG: Client connected\n");
            foxdbg_protocol_connect(wsi);

            lws_callback_on_writable(wsi);
        } break;

        case LWS_CALLBACK_RECEIVE:
        {
            foxdbg_protocol_receive((char *)in, len);
        } break;

        case LWS_CALLBACK_CLOSED:
        {
            printf("FOXDBG: Client disconnected\n");
            foxdbg_protocol_disconnect(wsi);
        } break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
        {
            
            /* handle server writeable event */

            auto start = std::chrono::high_resolution_clock::now();
            //foxdbg_procotol_send_pending(); // Assuming 'procotol' is the intended function name as in the original code
            foxdbg_protocol_transmit_subscriptions();

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            //printf("FOXDBG: foxdbg_procotol_send_pending took %lld ms\n", duration.count());
            
            lws_callback_on_writable(wsi);
        } break;

        default:
        {
            /* unhandled case */
        } break;
    }

    return 0;
}