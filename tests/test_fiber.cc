#include"../wubai/wubai.h"
#include<iostream>

wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

void run_in_fiber() {
    WUBAI_LOG_INFO(g_logger) << "run_in_fiber begin";
    wubai::Fiber::GetThis()->setState(wubai::Fiber::Stat::HOLD);
    wubai::Fiber::GetThis()->back();
    WUBAI_LOG_INFO(g_logger) << "run_in_fiber end";
    wubai::Fiber::GetThis()->setState(wubai::Fiber::Stat::HOLD);
    wubai::Fiber::GetThis()->back();
}

void test_fiber() {
    wubai::Fiber::GetThis();
    WUBAI_LOG_INFO(g_logger) << "main begin";
    wubai::Fiber::ptr fiber(new wubai::Fiber(run_in_fiber, 0, true));    //t_fiber = fiber(run_in_fiber); t_threadfiber(test_fiber);
    fiber->call();
    WUBAI_LOG_INFO(g_logger) << "main after swapIn";
    fiber->call();
    WUBAI_LOG_INFO(g_logger) << "main after end";
    fiber->call();
    WUBAI_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char** argv) {
    wubai::Thread::SetName("main");
    std::vector<wubai::Thread::ptr> thrs;

    for(int i = 0; i < 3; ++i) {
        thrs.push_back(wubai::Thread::ptr(new wubai::Thread(test_fiber, "name_" + std::to_string(i))));
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    return 0;
}