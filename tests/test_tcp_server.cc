#include"../wubai/wubai.h"


static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

void run() {
    auto addr = wubai::Address::LookupAny("0.0.0.0:8033");
    auto addr2 = wubai::UnixAddress::ptr(new wubai::UnixAddress("/tmp/unix_addr"));
    WUBAI_LOG_INFO(g_logger) << *addr << " - " << *addr2;
    std::vector<wubai::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    wubai::TcpServer::ptr tcp_server(new wubai::TcpServer);
    std::vector<wubai::Address::ptr> fails;
    tcp_server->bind(addrs, fails);
    tcp_server->start();

}

int main(int argc, char** argv) {
    wubai::IOManager iom(2);
    iom.schedule(run);
    return 0;
}