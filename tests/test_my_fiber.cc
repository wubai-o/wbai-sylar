#include"../wubai/fiber.h"
#include<iostream>
#include"../wubai/log.h"

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

void test2() {
    WUBAI_LOG_INFO(g_logger) << "test1";
}

void test1() {
    wubai::Fiber::ptr fiber2(new wubai::Fiber(test2));
    WUBAI_LOG_INFO(g_logger) << "test1";
    fiber2->call();
}

int main(int argc, char** argv) {

    return 0;
}