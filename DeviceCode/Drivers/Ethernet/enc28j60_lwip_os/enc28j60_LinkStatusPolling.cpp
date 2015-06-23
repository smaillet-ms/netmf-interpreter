#include "enc28j60_lwip.h"

HAL_CONTINUATION InterruptTaskContinuation;
HAL_COMPLETION LinkStatusCompletion;
BOOL LwipNetworkStatus = FALSE;

extern netif g_ENC28J60_NetIF;

void lwip_interrupt_continuation( )
{
    NATIVE_PROFILE_PAL_NETWORK();
    GLOBAL_LOCK(irq);
    
    if(!InterruptTaskContinuation.IsLinked())
    {
        InterruptTaskContinuation.Enqueue();
    }
}

static void LinkStatusPollHandler( void *arg )
{
    NATIVE_PROFILE_PAL_NETWORK( );

    struct netif* pNetIf = ( struct netif* )arg;

    BOOL status = enc28j60_get_link_status( &g_ENC28J60_LWIP_Config.DeviceConfigs[ 0 ].SPI_Config );

    if( status != LwipNetworkStatus )
    {
        if( status )
        {
            LWIP::SetLinkUp( pNetIf );
            LWIP::SetNetworkUp( pNetIf );
        }
        else
        {
            LWIP::SetLinkDown( pNetIf );
            LWIP::SetNetworkDown( pNetIf );
        }

        LwipNetworkStatus = status;
    }

    LinkStatusCompletion.EnqueueDelta64( 2000000 );
}

void InitContinuations( netif* pNetIf )
{
    InterruptTaskContinuation.InitializeCallback( (HAL_CALLBACK_FPN)enc28j60_lwip_interrupt, &g_ENC28J60_NetIF );
    LinkStatusCompletion.InitializeForUserMode( ( HAL_CALLBACK_FPN )LinkStatusPollHandler, pNetIf );
    LinkStatusCompletion.EnqueueDelta64( 500000 );
}

