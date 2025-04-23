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

#include "foxdbg.h"
#include "foxdbg_channels.h"
#include "foxdbg_thread.h"
#include "foxdbg_protocol.h"

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

static void foxglove_thread_main();

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

static std::thread foxglove_thread;
static std::atomic<bool> running = true;

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void foxdbg_thread_init()
{   
    running = true;

    foxglove_thread = std::thread(foxglove_thread_main);
    foxglove_thread.detach();
}

void foxdbg_thread_shutdown(void)
{

    running = false;
    
    lws_cancel_service(context);

    if (foxglove_thread.joinable())
    {
        foxglove_thread.join();
    }

}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/


void foxglove_thread_main()
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

    foxdbg_protocol_init(context);

    printf("FOXDBG: Server started on port %d\n", FOXDBG_PORT);

    while (running)
    {
        if (context != NULL) 
        {
            lws_service(context, 0);
        }
    }

    lws_context_destroy(context);
}

static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    
    if (!running) {
        lws_set_timeout(wsi, PENDING_TIMEOUT_USER_OK, LWS_TO_KILL_SYNC);
        return -1;
    }

    switch (reason) {
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
            foxdbg_procotol_send_pending(); // Assuming 'procotol' is the intended function name as in the original code
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