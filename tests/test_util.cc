#include"../wubai/wubai.h"
#include<assert.h>


wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

void test_assert() {
    WUBAI_LOG_INFO(g_logger) << wubai::BackTraceToString(10);
    //WUBAI_ASSERT(false);
    WUBAI_ASSERT_STRING(false, "abcdefg");
}

int main(int argc, char** argv) {
    test_assert();
    return 0;
}