#include"tcp_server.h"
#include"config.h"
#include"log.h"

namespace wubai {

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

static wubai::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = 
    wubai::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2), "tcp server read timeout");


TcpServer::TcpServer(wubai::IOManager* worker, wubai::IOManager* accept_worker) 
    :m_worker(worker)
    ,m_acceptWorker(accept_worker)
    ,m_recvTimeout(g_tcp_server_read_timeout->getValue())
    ,m_name("wubai/1.0.0")
    ,m_isStop(true) {
}

bool TcpServer::bind(wubai::Address::ptr addr) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fail_addrs) {
    for(auto& addr : addrs) {
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock->bind(addr)) {
            WUBAI_LOG_ERROR(g_logger) << "bind fail errno = " << errno << " errorstr " << strerror(errno) << "addr = [" << addr->toString() << "]";
            fail_addrs.push_back(addr);
            continue;
        }
        if(!sock->listen()) {
            WUBAI_LOG_ERROR(g_logger) << "listen fail errno = " << errno << "error str " << strerror(errno) << "addr = [ " << addr->toString() << " ] ";
            fail_addrs.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    }
    if(!fail_addrs.empty()) {
        m_socks.clear();
        return false;
    }
    for(auto& i : m_socks) {
        WUBAI_LOG_INFO(g_logger) << "server bind success: " << *i;
    }
    return true;
}

TcpServer::~TcpServer() {
    for(auto& sock : m_socks) {
        sock->close();
    }
    m_socks.clear();
}

void TcpServer::startAccpet(Socket::ptr sock) {
    while(!m_isStop) {
        Socket::ptr client = sock->accept();
        if(client) {
            client->setRecvTimeout(m_recvTimeout);
            m_worker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
        } else {
            WUBAI_LOG_ERROR(g_logger) << "accept errno = " << errno << "errorstr = " << strerror(errno);
        }
    }
}


bool TcpServer::start() {
    if(!m_isStop) {
        return true;
    }
    m_isStop = false;
    for(auto& sock : m_socks) {
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccpet, shared_from_this(), sock));
    }
    return true;
}

void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self]() {
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TcpServer::handleClient(Socket::ptr client) {
    WUBAI_LOG_INFO(g_logger) << "handleClient: " << *client;
}




}