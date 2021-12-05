#ifndef __WUBAI_MACRO_H__
#define __WUBAI_MACRO_H__

#include<string.h>
#include<assert.h>
#include"utils.h"

#if defined __GNUC__ || defined __llvm__
#   define WUBAI_LIKELY(x)     __builtin_expect(!!(x), 1)      //x为1的情况更多
#   define WUBAI_UNLIKELY(x)     __builtin_expect(!!(x), 0)    //x为0的情况更多
#else
#   define WUBAI_LIKELY(x)  (x)
#   define WUBAI_LIKELY(x)  (x)
#endif

#define WUBAI_ASSERT(x) \
    if(WUBAI_UNLIKELY(!(x))) { \
        WUBAI_LOG_ERROR(WUBAI_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << wubai::BackTraceToString(100, 2, "    "); \
        assert(x); \
    }

#define WUBAI_ASSERT_STRING(x,w) \
    if(WUBAI_UNLIKELY(!(x))) { \
        WUBAI_LOG_ERROR(WUBAI_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w << "\n" \
            << "\nbacktrace:\n" \
            << wubai::BackTraceToString(100, 2, "    "); \
        assert(x); \
    }








#endif