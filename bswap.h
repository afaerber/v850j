/*
 * Author:
 *   Andreas Färber <andreas.faerber@web.de>
 */
#ifndef BSWAP_H
#define BSWAP_H


#ifdef __APPLE__

#include <CoreFoundation/CoreFoundation.h>
#define cpu_to_le16(x) CFSwapInt16HostToLittle(x)
#define cpu_to_le32(x) CFSwapInt32HostToLittle(x)
#define cpu_to_be16(x) CFSwapInt16HostToBig(x)
#define cpu_to_be32(x) CFSwapInt32HostToBig(x)
#define le16_to_cpu(x) CFSwapInt16LittleToHost(x)
#define le32_to_cpu(x) CFSwapInt32LittleToHost(x)
#define be16_to_cpu(x) CFSwapInt16BigToHost(x)
#define be32_to_cpu(x) CFSwapInt32BigToHost(x)

#elif defined(__linux__)

#include <byteswap.h>
//#define __USE_BSD
#include <endian.h>
#define cpu_to_le16(x) htole16(x)
#define cpu_to_le32(x) htole32(x)
#define cpu_to_be16(x) htobe16(x)
#define cpu_to_be32(x) htobe32(x)
#define le16_to_cpu(x) le16toh(x)
#define le32_to_cpu(x) le32toh(x)
#define be16_to_cpu(x) be16toh(x)
#define be32_to_cpu(x) be32toh(x)

#else
#error Byte swapping not yet implemented for this platform
#endif


#endif
