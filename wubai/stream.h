#ifndef __WUBAI_STREAM_H__
#define __WUBAI_STREAM_H__

#include<memory>
#include"bytearray.h"

namespace wubai {

class Stream {
public:
    typedef std::shared_ptr<Stream> ptr;
    Stream() = default;
    virtual ~Stream() {}

    virtual int read(void* buff, size_t len) = 0;
    virtual int read(ByteArray::ptr ba, size_t len) = 0;
    virtual int readFixSize(void* buff, size_t len);
    virtual int readFixSize(ByteArray::ptr ba, size_t len);

    virtual int write(const void* buff, size_t len) = 0;
    virtual int write(ByteArray::ptr ba, size_t len) = 0 ;
    virtual int writeFixSize(const void* buff, size_t len);
    virtual int writeFixSize(ByteArray::ptr ba, size_t len);

    virtual void close() = 0;

};













}

#endif