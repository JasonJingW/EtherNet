#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

void network_lwip_assert(const char *message);

#define LWIP_PLATFORM_ASSERT(message) network_lwip_assert(message)
#define LWIP_PLATFORM_DIAG(args) ((void)0)

#endif
