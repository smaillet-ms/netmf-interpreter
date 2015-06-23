#include "tinyhal.h" 

#include "lwip\netifapi.h"
#include "lwip\pbuf.h"
#include "lwip\dhcp.h"
#include "lwip\tcpip.h"

namespace LWIP
{
    // handle ASYNC/SYNC calls based on support for multi threaded NETIF API
    // NETIFAPI calls use an async dispatcher pattern (via a mail box) to trigger
    // execution of a function on the main network stack thread
    // Putting these all as inlined functions in one place saves on cluttering the
    // code with repeated preprocessor checks for each configuration, which can
    // otherwise cause confusion for the reader as well as creating opportunities
    // for subtle copy/paste errors.
    #if LWIP_NETIF_API
    // multi-threaded OS
        inline void SetLinkUp( struct netif* pNetIf ) { netifapi_netif_common( pNetIf, netif_set_link_up, NULL); }
        inline void SetLinkDown( struct netif* pNetIf ) { netifapi_netif_common( pNetIf, netif_set_link_down, NULL); }
    #  if LWIP_DHCP
        inline void DhcpStart( struct netif* pNetIf ) { netifapi_dhcp_start( pNetIf ); }
        inline void SetNetworkUp( struct netif* pNetIf ) { netifapi_netif_set_up( pNetIf ); }
        inline void SetNetworkDown( struct netif* pNetIf ) { netifapi_netif_set_down( pNetIf ); }
    #  else
        inline void DhcpStart( struct netif* pNetIf ) { /*NOP*/ }
        inline void SetNetworkUp( struct netif* pNetIf ) { /*NOP*/ }
        inline void SetNetworkDown( struct netif* pNetIf ) { /*NOP*/ }
    #  endif        
    #else        
    // single-threaded OS
        inline void SetLinkUp( struct netif* pNetIf ) { netif_set_link_up( pNetIf ); }
        inline void SetLinkDown( struct netif* pNetIf ) { netif_set_link_down( pNetIf ); }
    #  if LWIP_DHCP
        inline void DhcpStart( struct netif* pNetIf ) { dhcp_start( pNetIf ); }
        inline void SetNetworkUp( struct netif* pNetIf ) { netif_set_up( pNetIf ); }
        inline void SetNetworkDown( struct netif* pNetIf ) { netif_set_down( pNetIf ); }
    #  else
        inline void DhcpStart( struct netif* pNetIf ) { /*NOP*/ }
        inline void SetNetworkUp( struct netif* pNetIf ) { /*NOP*/ }
        inline void SetNetworkDown( struct netif* pNetIf ) { /*NOP*/ }
    #  endif
    #endif

    // Forward a frame contained in a pbuf to the lwip stack.
    // 
    // Input:
    //    pNetif - the lwip network interface.
    //      pbuf - the pbuf containing the frame.
    // Remarks:
    //   This uses preprocessor conditional (LWIP_NETIF_API)
    //   to implement the correct behavior depending on whether
    //   a system is built with or without an underlying
    //   multi-threaded OS.
    inline void PostRxFrame(struct netif* pNetif, struct pbuf* pbuf )
    {
    #if LWIP_NETIF_API
        // post the input packet to the stack
        tcpip_input( pbuf, pNetif );
    #else
        // Transmit the frame directly to the lwip stack
        // and process it on the current thread.
        err_t err = pNetif->input(pbuf, pNetif);
        if (err != ERR_OK)
        {
            pbuf_free(pbuf);
            pbuf = NULL;
        }
    #endif
    }
}
