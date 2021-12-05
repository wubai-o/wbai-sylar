#ifndef __WUBAI_ENDIAN_H__
#define __WUBAI_ENDIAN_H__

#define WUBAI_LITTLE_ENDIAN 1
#define WUBAI_BIG_ENDIAN 2

#include<byteswap.h>
#include<stdint.h>

namespace wubai {

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type 
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type 
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type 
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define WUBAI_BYTE_ORDER WUBAI_BIG_ENDIAN
#else
#define WUBAI_BYTE_ORDER WUBAI_LITTLE_ENDIAN
#endif

#if WUBAI_BYTE_ORDER == WUBAI_BIG_ENDIAN

template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}

template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

#else

template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}

template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

#endif





}



#endif