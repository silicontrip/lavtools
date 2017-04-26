#ifndef _BYTESWAP_H_
#define _BYTESWAP_H_


#include <sys/types.h>

#define __BYTE_ORDER __LITTLE_ENDIAN

#ifndef __BYTE_ORDER
# error "Aiee: __BYTE_ORDER not defined\n";
#endif


#define SWAP2(x) (((x>>8) & 0x00ff) |\
                  ((x<<8) & 0xff00))


#define SWAP4(x) (((x>>24) & 0x000000ff) |\
                  ((x>>8)  & 0x0000ff00) |\
                  ((x<<8)  & 0x00ff0000) |\
                  ((x<<24) & 0xff000000))


# define LILEND2(a) (a)
# define LILEND4(a) (a)
# define BIGEND2(a) SWAP2((a))
# define BIGEND4(a) SWAP4((a))


#endif
