#ifndef __BYTEORDER_H
#define __BYTEORDER_H
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define __LITTLE_ENDIAN_BITFIELD
#elif __BYTE_ORDER == __BIG_ENDIAN
#define __BIG_ENDIAN_BITFIELD
#endif

#endif /* __BYTEORDER_H */
