#include <endian.h>
#include <byteswap.h>

#ifndef BYTEORDER_H
#define BYTEORDER_H

#if BYTE_ORDER == LITTLE_ENDIAN
#  define le2cpu(x) x
#elif BYTE_ORDER == BIG_ENDIAN
#  define le2cpu bswap_32
#endif

#endif

// vim: ts=2 sw=2 et
