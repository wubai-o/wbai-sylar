#include"local_server.h"

namespace wubai {
namespace http {

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

Localserver::Localserver(IOManager* worker, IOManager* acceptWorker)
    :m_worker(worker)
    ,m_acceptWorker(worker){
}

bool Localserver::localbind(wubai::Address::ptr addr) {
    m_localsock = Socket::CreateTCP(addr);
    if(!m_localsock->bind(addr)) {
        WUBAI_LOG_ERROR(g_logger) << "bind fail errno = " << errno << " errorstr " << strerror(errno) << "addr = [" << addr->toString() << "]";
    }
    if(!m_localsock->listen()) {
        WUBAI_LOG_ERROR(g_logger) << "listen fail errno = " << errno << "error str " << strerror(errno) << "addr = [ " << addr->toString() << " ] ";
    }
    WUBAI_LOG_INFO(g_logger) << *addr;
    return true;
}

bool Localserver::nodebind(wubai::Address::ptr addr) {
    m_nodesock = Socket::CreateTCP(addr);
    if(!m_nodesock->bind(addr)) {
        WUBAI_LOG_ERROR(g_logger) << "bind fail errno = " << errno << " errorstr " << strerror(errno) << "addr = [" << addr->toString() << "]";
    }
    if(!m_nodesock->listen()) {
        WUBAI_LOG_ERROR(g_logger) << "listen fail errno = " << errno << "error str " << strerror(errno) << "addr = [ " << addr->toString() << " ] ";
    }
    WUBAI_LOG_INFO(g_logger) << *addr;
    return true;
}


bool Localserver::start() {
    m_mysql.reset(new Mysql("127.0.0.1", "root", "123456", "car_data", 3306));
    wubai::IPAddress::ptr localaddr = wubai::Address::LookupAnyIPAddress("172.16.0.9");
    localaddr->setPort(3333);
    localbind(localaddr);
    wubai::IPAddress::ptr nodeaddr = wubai::Address::LookupAnyIPAddress("127.0.0.1");
    nodeaddr->setPort(8003);
    nodebind(nodeaddr);
    Socket::ptr acceptsock = m_nodesock->accept();
    if(!acceptsock) {
        WUBAI_LOG_INFO(g_logger) << *nodeaddr << " accept failed";
    } else {
        WUBAI_LOG_INFO(g_logger) << *nodeaddr << " accept success";
    }
    m_nodeServer.reset(new SocketStream(acceptsock, false));
    m_acceptWorker->schedule(std::bind(&Localserver::startAccpet, shared_from_this(), m_localsock));
    m_worker->schedule(std::bind(&Localserver::recvmsg, shared_from_this()));
    return true;
}

void Localserver::startAccpet(Socket::ptr sock) {
    while(true) {
        Socket::ptr client = sock->accept();
        if(client) {
            WUBAI_LOG_INFO(g_logger) << "accept successful";
            m_worker->schedule(std::bind(&Localserver::handleClient, shared_from_this(), client));
        } else {
            WUBAI_LOG_ERROR(g_logger) << "accept errno = " << errno << "errorstr = " << strerror(errno);
        }
    }
}

void Localserver::handleClient(Socket::ptr client) {
    char buf[1024];
    m_localServer.reset(new SocketStream(client, false));
    int rt = 0;
    std::string str;
    while((rt = m_localServer->read(buf, 1024)) > 0) {
        std::stringstream ss;
        str = std::string(buf, rt);
        ss << str;
        std::string tmp;
        while(getline(ss, tmp, '\n')) {
            if(tmp.size() == 0) {
                break;
            }
            m_worker->schedule(std::bind(&Localserver::accessSQL, shared_from_this(), tmp));
            m_worker->schedule(std::bind(&Localserver::sendmsg, shared_from_this(), tmp));
        }
    }
}

void Localserver::accessSQL(std::string str) {
    m_mysql->addData(str);
}

void Localserver::sendmsg(std::string str) {
    m_nodeServer->write(str.c_str(), str.size());
}

void Localserver::recvmsg() {
    char buf[512];
    int rt = 0;
    while((rt = m_nodeServer->read(buf, 512)) > 0) {
        std::string str = std::string(buf, rt);
        std::cout << str << std::endl;
        m_localServer->write(str.c_str(), str.size());
    }
}


}
}