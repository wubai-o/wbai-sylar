#ifndef __WUBAI_LIBRARY_H__
#define __WUBAI_LIBRARY_H__

#include"module.h"

namespace wubai {

class Library {
public:
    static Module::ptr GetModule(const std::string& path);
};

}

#endif