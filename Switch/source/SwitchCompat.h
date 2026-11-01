/* Resolve dependencies that newlib/libnx doesn't have */
#pragma once

#include <limits.h>

#ifndef LLONG_MAX
#define LLONG_MAX 9223372036854775807LL
#endif

#ifndef LLONG_MIN
#define LLONG_MIN (-LLONG_MAX - 1LL)
#endif

#ifndef ULLONG_MAX
#define ULLONG_MAX 18446744073709551615ULL
#endif

#ifndef IN6ADDR_ANY_INIT
#define IN6ADDR_ANY_INIT { 0 }
#endif

#include <netinet/in.h>

#ifndef __SWITCH_IPV6_MREQ_DEFINED
#define __SWITCH_IPV6_MREQ_DEFINED
struct ipv6_mreq {
	struct in6_addr ipv6mr_multiaddr;
	unsigned int ipv6mr_interface;
};
#endif
