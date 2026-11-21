#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/* The network task owns all lwIP calls, including packet input and timeouts. */
#define NO_SYS (1)
#define SYS_LIGHTWEIGHT_PROT (0)
#define LWIP_SOCKET (0)
#define LWIP_NETCONN (0)
#define LWIP_IPV4 (1)
#define LWIP_IPV6 (0)
#define LWIP_ARP (1)
#define LWIP_ICMP (1)
#define LWIP_UDP (1)
#define LWIP_TCP (0)
#define LWIP_RAW (0)
#define LWIP_DHCP (0)
#define LWIP_AUTOIP (0)
#define LWIP_DNS (0)
#define LWIP_IGMP (0)
#define IP_REASSEMBLY (0)
#define IP_FRAG (0)
#define LWIP_STATS PROJECT_USE_RTT
#define MEM_ALIGNMENT (4)
#define MEM_SIZE (8 * 1024)
#define PBUF_POOL_SIZE (8)
#define PBUF_POOL_BUFSIZE (1536)

#endif
