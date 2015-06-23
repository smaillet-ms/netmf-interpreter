#include <tinyhal.h>
#include "enc28j60_lwip.h"

extern "C"
{
#include "netif\etharp.h"
#include "lwip\dhcp.h"
#include "lwip\tcpip.h"
#include "lwip\dns.h"
#include "lwip\ip_addr.h"
}

// NOTE: using a global scalar instance for the LWIP interface means this driver only supports a single instance
// (e.g. you cannot have more than one ENC28J60 physical chip connected to your system)
netif g_ENC28J60_NetIF;

static BOOL LwipNetworkStatus = FALSE;

extern ENC28J60_LWIP_DEVICE_CONFIG g_ENC28J60_LWIP_Config;
extern NETWORK_CONFIG g_NetworkConfig;

err_t enc28j60_ethhw_init( netif* pNetIf )
{
    pNetIf->mtu = ETHERSIZE;

    pNetIf->flags = NETIF_FLAG_IGMP | NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

    /* Assign the xmit routine to the stack's netif and call the driver's Open */
    pNetIf->output = etharp_output;
    pNetIf->linkoutput = enc28j60_lwip_xmit;

    enc28j60_lwip_setup_device( pNetIf );

    return ERR_OK;
}

int ENC28J60_LWIP_Driver::Open( ENC28J60_LWIP_DRIVER_CONFIG* config, int index )
{
    NATIVE_PROFILE_HAL_DRIVERS_ETHERNET( );

    /* Network interface variables */
    struct ip_addr ipaddr, netmask, gw;
    struct netif *pNetIF;
    int len;
    const SOCK_NetworkConfiguration *iface;

    if( config == NULL )
        return -1;

    iface = &g_NetworkConfig.NetworkInterfaces[ index ];

    if( 0 == ( iface->flags & SOCK_NETWORKCONFIGURATION_FLAGS_DHCP ) )
    {
        ipaddr.addr = iface->ipaddr;
        gw.addr = iface->gateway;
        netmask.addr = iface->subnetmask;
    }
    else
    {
        /* Set network address variables - this will be set by either DHCP or when the configuration is applied */
        IP4_ADDR( &gw, 0, 0, 0, 0 );
        IP4_ADDR( &ipaddr, 0, 0, 0, 0 );
        IP4_ADDR( &netmask, 255, 255, 255, 0 );
    }

    len = g_ENC28J60_NetIF.hwaddr_len;

    if( len == 0 || iface->macAddressLen < len )
    {
        len = iface->macAddressLen;
        g_ENC28J60_NetIF.hwaddr_len = len;
    }

    memcpy( g_ENC28J60_NetIF.hwaddr, iface->macAddressBuffer, len );

    pNetIF = netif_add( &g_ENC28J60_NetIF, &ipaddr, &netmask, &gw, NULL, enc28j60_ethhw_init, ethernet_input );

    netif_set_default( pNetIF );

    LwipNetworkStatus = enc28j60_get_link_status( &config->SPI_Config );

    /* Initialize the continuation routine for the driver interrupt and receive */
    InitContinuations( pNetIF );

    /* Enable the INTERRUPT pin */
    if( CPU_GPIO_EnableInputPin2( config->INT_Pin,
                                  FALSE,                                            /* Glitch filter disabled */
                                  ( GPIO_INTERRUPT_SERVICE_ROUTINE )&enc28j60_isr,  /* ISR                    */
                                  &g_ENC28J60_NetIF,                                /* argument for handler   */
                                  GPIO_INT_EDGE_LOW,                                /* Interrupt edge         */
                                  RESISTOR_PULLUP ) == FALSE )                      /* Resistor State         */
    {
        return -1;
    }

    /* Enable the CHIP SELECT pin */
    if( CPU_GPIO_EnableInputPin( config->SPI_Config.DeviceCS,
                                 FALSE,
                                 NULL,
                                 GPIO_INT_NONE,
                                 RESISTOR_PULLUP ) == FALSE )
    {
        return -1;
    }

    return g_ENC28J60_NetIF.num;

}

BOOL ENC28J60_LWIP_Driver::Close( ENC28J60_LWIP_DRIVER_CONFIG* config, int index )
{
    NATIVE_PROFILE_HAL_DRIVERS_ETHERNET( );

    if( config == NULL )
        return FALSE;

    LinkStatusCompletion.Abort( );

    LWIP::SetLinkDown( &g_ENC28J60_NetIF );
    LWIP::SetNetworkDown( &g_ENC28J60_NetIF );
    netif_remove( &g_ENC28J60_NetIF );

    /* Disable the INTERRUPT pin */
    CPU_GPIO_EnableInputPin2( config->INT_Pin,
                              FALSE,                         /* Glitch filter enable */
                              NULL,                          /* ISR                  */
                              0,                             /* minor number         */
                              GPIO_INT_NONE,                 /* Interrupt edge       */
                              RESISTOR_PULLUP );             /* Resistor State       */

    InterruptTaskContinuation.Abort( );

    LwipNetworkStatus = FALSE;

    enc28j60_lwip_close( &g_ENC28J60_NetIF );

    memset( &g_ENC28J60_NetIF, 0, sizeof( g_ENC28J60_NetIF ) );

    return TRUE;
}

BOOL  ENC28J60_LWIP_Driver::Bind( ENC28J60_LWIP_DRIVER_CONFIG* config, int index )
{
    NATIVE_PROFILE_HAL_DRIVERS_ETHERNET( );

    return TRUE;
}

