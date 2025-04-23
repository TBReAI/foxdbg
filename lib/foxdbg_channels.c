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

#include "foxdbg_channels.h"

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
** MARK: VARIABLES
***************************************************************/

foxdbg_channel_t tx_channels[] = {
    /* SENSORS */

    {
        .topic = "/sensors/camera",
        .channel_type = FOXDBG_CHANNEL_TYPE_IMAGE,
        FOXDBG_CHANNEL_EMPTY
    },
    {
        .topic = "/sensors/depth",
        .channel_type = FOXDBG_CHANNEL_TYPE_IMAGE,
        FOXDBG_CHANNEL_EMPTY
    },
    {
        .topic = "/sensors/lidar",
        .channel_type = FOXDBG_CHANNEL_TYPE_POINTCLOUD,
        FOXDBG_CHANNEL_EMPTY
    },

    /* PERCEPTION */
    {
        .topic = "/perception/object_detection",
        .channel_type = FOXDBG_CHANNEL_TYPE_IMAGE,
        FOXDBG_CHANNEL_EMPTY
    },
    {
        .topic = "/perception/lidar_surface_boxes",
        .channel_type = FOXDBG_CHANNEL_TYPE_CUBES,
        FOXDBG_CHANNEL_EMPTY
    },
    {
        .topic = "/perception/lidar_cluster_boxes",
        .channel_type = FOXDBG_CHANNEL_TYPE_CUBES,
        FOXDBG_CHANNEL_EMPTY
    },
    {
        .topic = "/perception/output",
        .channel_type = FOXDBG_CHANNEL_TYPE_CUBES,
        FOXDBG_CHANNEL_EMPTY
    },

    /* NAVIGATION */
    {
        .topic = "/navigation/odometry_pose",
        .channel_type = FOXDBG_CHANNEL_TYPE_POSE,
        FOXDBG_CHANNEL_EMPTY
    },
    {
        .topic = "/navigation/slam_map",
        .channel_type = FOXDBG_CHANNEL_TYPE_CUBES,
        FOXDBG_CHANNEL_EMPTY
    },
    {
        .topic = "/navigation/path_planning",
        .channel_type = FOXDBG_CHANNEL_TYPE_LINES,
        FOXDBG_CHANNEL_EMPTY
    },

    /* CONTROL */
    {
        .topic = "/control/vehicle_request",
        .channel_type = FOXDBG_CHANNEL_TYPE_FLOAT,
        FOXDBG_CHANNEL_EMPTY
    },
    {
        .topic = "/control/vehicle_feedback",
        .channel_type = FOXDBG_CHANNEL_TYPE_FLOAT,
        FOXDBG_CHANNEL_EMPTY
    },

    /* SYSTEM */
    {
        .topic = "/system/state",
        .channel_type = FOXDBG_CHANNEL_TYPE_BOOLEAN,
        FOXDBG_CHANNEL_EMPTY
    },

    /* OTHER */
    {
        .topic = "/reference/sine",
        .channel_type = FOXDBG_CHANNEL_TYPE_FLOAT,
        FOXDBG_CHANNEL_EMPTY
    },

};

foxdbg_channel_t rx_channels[] = {
    
    /* SYSTEM */
    {
        .topic = "/rx/system_state",
        .channel_type = FOXDBG_CHANNEL_TYPE_BOOLEAN,
        FOXDBG_CHANNEL_EMPTY
    },
};


/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/

foxdbg_channel_t* foxdbg_get_tx_channels(void)
{
    return tx_channels;
}

size_t foxdbg_get_tx_channels_count(void)
{
    return sizeof(tx_channels) / sizeof(foxdbg_channel_t);
}

foxdbg_channel_t* foxdbg_get_rx_channels(void)
{
    return rx_channels;
}

size_t foxdbg_get_rx_channels_count(void)
{
    return sizeof(rx_channels) / sizeof(foxdbg_channel_t);
}


/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/

