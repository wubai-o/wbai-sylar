#include"../wubai/wubai.h"

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

void test_socket() {
    wubai::IPAddress::ptr addr = wubai::Address::LookupAnyIPAddress("www.baidu.com");
    if(addr) {
        WUBAI_LOG_INFO(g_logger) << " get address: " << addr->toString();
    } else {
        WUBAI_LOG_ERROR(g_logger) << " get address failed ";
        return;
    }
    wubai::Socket::ptr sock = wubai::Socket::CreateTCP(addr);
    addr->setPort(80);
    if(!sock->connect(addr)) {
        WUBAI_LOG_ERROR(g_logger) << " connect " << addr->toString() << "fail";
    } else {
        WUBAI_LOG_INFO(g_logger) << " connect " << addr->toString() << "success";
    }
    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        WUBAI_LOG_ERROR(g_logger) << "send fail rt = " << rt;
        return;
    }
    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0],buffs.size());

    if(rt <= 0) {
        WUBAI_LOG_ERROR(g_logger) << "recv fail rt = " << rt;
        return;
    }
    buffs.resize(rt);
    WUBAI_LOG_INFO(g_logger) << buffs;
}

int main(int argc, char** argv) {
    wubai::IOManager iom;
    iom.schedule(test_socket);
    return 0;
}