#include"../wubai/wubai.h"

wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

void test() {
    std::vector<wubai::Address::ptr> addrs;
    bool v = wubai::Address::Lookup(addrs, "www.baidu.com:ftp");
    if(!v) {
        WUBAI_LOG_ERROR(g_logger) << "lookup failed";
    }
    for(size_t i = 0; i < addrs.size(); ++i) {
        WUBAI_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_interface() {
    std::multimap<std::string, std::pair<wubai::Address::ptr, uint32_t> > results;
    bool v = wubai::Address::GetInterfaceAddresses(results);
    if(!v) {
        WUBAI_LOG_ERROR(g_logger) << "GetInterfaceAddress failed";
        return;
    }
    for(auto& i : results) {
        WUBAI_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - " << i.second.second;
    }
}

void test_ipv4() {
    auto addr = wubai::IPAddress::Create("127.0.0.8");
    if(addr) {
        WUBAI_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    test_ipv4();
    //test();
    //test_interface();
    return 0;
}