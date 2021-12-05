#include"../wubai/wubai.h"

wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

int count = 0;

void fun1() {
    WUBAI_LOG_INFO(g_logger) << "name" << wubai::Thread::GetName()
                             << "this.name" << wubai::Thread::GetThis()->getName() 
                             << "id" << wubai::GetThreadId()
                             << "this.id" << wubai::Thread::GetThis()->getId();
    for(int i = 0; i < 100000; ++i) {
        ++count;
    }
}

void fun2() {
    while(true) {
        WUBAI_LOG_INFO(g_logger) << "======================";
    } 
}

void fun3() {
    while(true) {
        WUBAI_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxx";
    }
}

int main() {
    WUBAI_LOG_INFO(g_logger) << "test begin";
    YAML::Node node = YAML::LoadFile("/root/wubai/log2.yml");
    wubai::Config::LoadFromYaml(node);
    std::vector<wubai::Thread::ptr> thrs;

    for(int i = 0; i < 0; ++i) {
        wubai::Thread::ptr thr(new wubai::Thread(&fun2, "name_" + std::to_string(i * 2)));
        wubai::Thread::ptr thr2(new wubai::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }

    WUBAI_LOG_INFO(g_logger) << "test end";
    return 0;
}