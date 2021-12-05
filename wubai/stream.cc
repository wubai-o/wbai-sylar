#include"stream.h"

namespace wubai {

int Stream::readFixSize(void* buff, size_t len) {
    size_t offset = 0;
    size_t left = len;
    while(left > 0) {
        size_t rt = read((char*)buff + offset, left);
        if(rt <= 0) {
            return rt;
        }
        offset += rt;
        left -= rt;
    }
    return len;
}

int Stream::readFixSize(ByteArray::ptr ba, size_t len) {
    size_t left = len;
    while(left > 0) {
        size_t rt = read(ba, left);
        if(rt <= 0) {
            return rt;
        }
        left -= rt;
    }
    return len;
}

int Stream::writeFixSize(const void* buff, size_t len) {
    size_t offset = 0;
    size_t left = len;
    while(left > 0) {
        size_t rt = write((const char*)buff + offset, left);
        if(len <= 0) {
            return rt;
        }
        offset += rt;
        left -= rt;
    }
    return len;
}

int Stream::writeFixSize(ByteArray::ptr ba, size_t len) {
    size_t left = len;
    while(left > 0) {
        size_t rt = write(ba, left);
        if(rt <= 0) {
            return rt;
        }
        left -= rt;
    }
    return len;
}







}