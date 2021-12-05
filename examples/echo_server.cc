#include"../wubai/wubai.h"
#include<iostream>

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

class EchoServer : public wubai::TcpServer {
public:
    EchoServer(int type);
    void handleClient(wubai::Socket::ptr client) override;

private:
    int m_type = 0; 
};

EchoServer::EchoServer(int type) 
    :m_type(type) {
}

void EchoServer::handleClient(wubai::Socket::ptr client) {
    WUBAI_LOG_INFO(g_logger) << "handleClient" << *client;
    wubai::ByteArray::ptr ba(new wubai::ByteArray);
    while(true) {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);
        int rt = client->recv(&iovs[0], iovs.size());
        if(rt == 0) {
            WUBAI_LOG_INFO(g_logger) << "client close:" << *client;
            break;
        } else if(rt < 0) {
            WUBAI_LOG_INFO(g_logger) << " client error rt = " << rt << " errno = " << errno << " error str " << strerror(errno);
            break;
        }

        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        if(m_type == 1) {//text
            std::cout << ba->toString() << std::endl;
        } else {
            std::cout << ba->toHexString() << std::endl;
        }
    }

}

int type = 1;

void run() {
    WUBAI_LOG_INFO(g_logger) << "server type = " << type;
    EchoServer::ptr es(new EchoServer(type));;
    auto addr = wubai::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)) {
        sleep(10);
    }
    es->start();
}

int main(int argc, char** argv) {
    if(argc < 2) {
        WUBAI_LOG_INFO(g_logger) << "used as[" << argv[0] << "-t ] or [ " << argv[0] << " -b]";
        return 0;
    }
    
    if(!strcmp(argv[1], "-b")) {
        type = 2;
    }

    wubai::IOManager iom;
    iom.schedule(run);
    return 0;
}