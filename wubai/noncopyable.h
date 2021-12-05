#ifndef __WUBAI_NONCOPYABLE_H__
#define __WUBAI_NONCOPYABLE_H__

namespace wubai {

class Noncopyable {
protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
private:
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;

};




}






#endif