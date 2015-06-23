#include "enc28j60_lwip_driver.h"

BOOL Network_Interface_Bind( int index )
{
    NATIVE_PROFILE_HAL_DRIVERS_ETHERNET( );
    if( index >= ARRAYSIZE( g_ENC28J60_LWIP_Config.DeviceConfigs ) )
        return FALSE;

    return ENC28J60_LWIP_Driver::Bind( &g_ENC28J60_LWIP_Config.DeviceConfigs[ index ], index );
}

int Network_Interface_Open( int index )
{
    NATIVE_PROFILE_HAL_DRIVERS_ETHERNET( );
    if( index >= ARRAYSIZE( g_ENC28J60_LWIP_Config.DeviceConfigs ) )
        return -1;

    HAL_CONFIG_BLOCK::ApplyConfig( ENC28J60_LWIP_DEVICE_CONFIG::GetDriverName( ), &g_ENC28J60_LWIP_Config, sizeof( g_ENC28J60_LWIP_Config ) );

    return ENC28J60_LWIP_Driver::Open( &g_ENC28J60_LWIP_Config.DeviceConfigs[ index ], index );
}

BOOL Network_Interface_Close( int index )
{
    NATIVE_PROFILE_HAL_DRIVERS_ETHERNET( );
    if( index >= ARRAYSIZE( g_ENC28J60_LWIP_Config.DeviceConfigs ) )
        return FALSE;

    return ENC28J60_LWIP_Driver::Close( &g_ENC28J60_LWIP_Config.DeviceConfigs[ index ], index );
}
