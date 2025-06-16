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
#include "foxdbg_thread.h"
#include "foxdbg_buffer.h"

#include "foxdbg_protocol.h"

#include <libwebsockets.h>
#include <stdio.h>

#include <chrono>
#include <thread>
#include <atomic>

#ifdef WIN32
    #include <windows.h>
    #include <process.h>
#else
    #include <pthread.h>
    #include <sched.h>
    #include <unistd.h>
    #include <sys/sysinfo.h>
#endif



/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#define PRIORITY_LOW        (1U)
#define PRIORITY_NORMAL     (2U)
#define PRIORITY_HIGH       (3U)
#define PRIORITY_CRITICAL   (4U)

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/


/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

static int foxdbg_server_thread_main();
static int foxdbg_encoder_thread_main();

static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

static size_t get_core_count(void);
static void set_core(size_t core_id);
static void set_thread_priority(int priority);

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

static std::atomic_bool running(false);

static foxdbg_channel_t **channels = NULL;
static size_t *channel_count = NULL;

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

void foxdbg_thread_init(foxdbg_channel_t **channels_ptr, size_t *channel_count_ptr)
{   
    running.store(true);

    channels = channels_ptr;
    channel_count = channel_count_ptr;

    foxdbg_server_thread = std::thread(foxdbg_server_thread_main);
}

void foxdbg_thread_shutdown(void)
{
    lws_cancel_service(context);

    running.store(false);

    try
    {
        if (foxdbg_server_thread.joinable())
        {
            foxdbg_server_thread.join();
        }
    }
    catch (...)
    {
        /* thread error */
    }
    
}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

int foxdbg_server_thread_main() 
{

    set_core(1);  // Set to core 0
    set_thread_priority(PRIORITY_HIGH);

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
    }

    foxdbg_protocol_shutdown();

    printf("Server thread exiting...\n");
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

            foxdbg_protocol_transmit_subscriptions();

            lws_callback_on_writable(wsi);
        } break;

        default:
        {
            /* unhandled case */
        } break;
    }

    return 0;
}


#ifdef WIN32
    static size_t get_core_count() 
    {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        return sysinfo.dwNumberOfProcessors;
    }

    static void set_core(size_t core_id) 
    {
        SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)(1 << core_id));
        printf("SET AFFINITY TO %zu\n", core_id);
    }

    static void set_thread_priority(int priority)
    {
        int win_priority;
        HANDLE thread = GetCurrentThread();

        switch (priority) 
        {
            case PRIORITY_LOW:
            {
                win_priority = THREAD_PRIORITY_BELOW_NORMAL;
            } break;
                
            case PRIORITY_NORMAL:
            {
                win_priority = THREAD_PRIORITY_NORMAL;
            } break;
            
            case PRIORITY_HIGH:
            {
                win_priority = THREAD_PRIORITY_ABOVE_NORMAL;
            } break;
            
            case PRIORITY_CRITICAL:
            {
                win_priority = THREAD_PRIORITY_HIGHEST;
            } break;
                
            default:
            {
                win_priority = THREAD_PRIORITY_NORMAL;
            } break;
        }

        if (!SetThreadPriority(thread, win_priority)) 
        {
            fprintf(stderr, "Failed to set thread priority. Error: %d\n", GetLastError());
        } 
        else 
        {
            printf("Set thread priority (Windows) to %d\n", win_priority);
        }
    }

#else

    static size_t get_core_count() 
    {
        struct sysinfo info;
        sysinfo(&info);
        return info.procs;
    }

    static void set_core(size_t core_id) 
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);

        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
        printf("SET AFFINITY TO %zu\n", core_id);
    }

    static void set_thread_priority(int priority)
    {
        struct sched_param sched;
        int policy = SCHED_RR;
        int posix_priority;

        pthread_t this_thread = pthread_self();

        int min_priority = sched_get_priority_min(policy);
        int max_priority = sched_get_priority_max(policy);

        switch (priority) 
        {
            case PRIORITY_LOW:
            {
                posix_priority = min_priority;
            } break;
            
            case PRIORITY_NORMAL:
            {
                posix_priority = (min_priority + max_priority) / 2;
            } break;
                
            case PRIORITY_HIGH:
            {
                posix_priority = max_priority - 10;
            } break;
                
            case PRIORITY_CRITICAL:
            {
                posix_priority = max_priority;
            } break;

            default:
            {
                posix_priority = (min_priority + max_priority) / 2;
            } break;
        }

        sched.sched_priority = posix_priority;

        if (pthread_setschedparam(this_thread, policy, &sched) != 0) 
        {
            perror("pthread_setschedparam");
            fprintf(stderr, "You might need elevated privileges to set real-time priority.\n");
        }
        else 
        {
            printf("Set thread priority (POSIX) to %d with policy SCHED_RR\n", posix_priority);
        }
    }
#endif