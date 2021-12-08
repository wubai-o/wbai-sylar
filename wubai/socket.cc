#include"socket.h"
#include"hook.h"
#include"log.h"
#include"fd_manager.h"
#include<netinet/tcp.h>
#include"macro.h"
#include<sys/types.h>
#include<sys/socket.h>
#include<iostream>

namespace wubai {

wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

Socket::ptr Socket::CreateTCP(wubai::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDP(wubai::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;    
}
Socket::ptr Socket::CreateUDPSocket6() {
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    return sock; 
}

Socket::ptr Socket::CreateUnixTCPSocket() {
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUnixUDPSocket() {
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}

Socket::Socket(int family, int type, int protocol)
    :m_sock(-1)
    ,m_family(family)
    ,m_type(type)
    ,m_protocol(protocol)
    ,m_isconnected(false) {
}

Socket::~Socket() {
    close();
}

int64_t Socket::getSendTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}

void Socket::setSendTimeout(int64_t v) {
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    std::cout << sizeof(tv) << std::endl;
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

void Socket::setRecvTimeout(int64_t v) {
    struct timeval tv{v / 1000, v % 1000 * 1000};
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void* result, size_t* len) {
    int rt = getsockopt(m_sock, level, option,  result, (socklen_t*)len);
    if(rt) {
        WUBAI_LOG_DEBUG(g_logger) << "getOption sock =  " << m_sock << " level = " << level << " option = " << option
            << " errno = " << errno << " strerror = " << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::setOption(int level, int option, const void* result, size_t len) {
    if(setsockopt(m_sock, level, option, result, (socklen_t)len)) {
        WUBAI_LOG_DEBUG(g_logger) << "getOption sock =  " << m_sock << " level = " << level << " option = " << option
            << " errno = " << errno << " strerror = " << strerror(errno);
        return false;
    }

    return true;
}

Socket::ptr Socket::accept() {
    //新socket对象,用来存放accept后的SocketFd
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if(newsock == -1) {
        WUBAI_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno = " << errno << "errorstr = " << strerror(errno);
        return nullptr;
    }
    //用accept生成的fd初始化socket,并且在
    if(sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

bool Socket::init(int sock) {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock, true);
    if(ctx && ctx->isSocket() && !ctx->isClose()) {
        m_sock = sock;
        m_isconnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

bool Socket::bind(const Address::ptr addr) {
    if(!isValid()) {
        newSock();
        if(WUBAI_UNLIKELY(!isValid())) {
            return false;
        }
    }
    if(WUBAI_UNLIKELY(addr->getFamily() != m_family)) {
        WUBAI_LOG_ERROR(g_logger) << "bind sock family ( " << m_family << ") addr family ( " << addr->getFamily() << " ) noequal, addr=" << addr->toString();
        return false; 
    }
    if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        WUBAI_LOG_ERROR(g_logger) << "bind errno" << errno << "errorstr = " << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    if(!isValid()) {
        newSock();
        if(WUBAI_UNLIKELY(!isValid())) {
            return false;
        }
    }
    if(WUBAI_UNLIKELY(addr->getFamily() != m_family)) {
        WUBAI_LOG_ERROR(g_logger) << "connect sock family ( " << m_family << ") addr family ( " << addr->getFamily() << " ) noequal, addr=" << addr->toString();
        return false; 
    }

    if(timeout_ms == (uint64_t)-1) {
        if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            WUBAI_LOG_ERROR(g_logger) << " sock= " << m_sock << " connect ( " << addr->toString()
                << " ) errno = " << errno << " errorstr " << strerror(errno);
            close();
            return false;
        }
    } else {
        if(connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
            WUBAI_LOG_ERROR(g_logger) << " sock= " << m_sock << " connect ( " << addr->toString()
                << " ) timeout = " << timeout_ms << "errno = " << errno << " errorstr " << strerror(errno);
            close();
            return false;  
        }
    }
    m_isconnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}

bool Socket::listen(int backlog) {
    if(!isValid()) {
        WUBAI_LOG_ERROR(g_logger) << "listen error sock = -1";
        return false;
    }
    if(WUBAI_UNLIKELY(::listen(m_sock, backlog))) {
        WUBAI_LOG_ERROR(g_logger) << " listen error errno = " << errno << " errorstr = " << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::close() {
    if(!m_isconnected && !isValid()) {
        return true;
    }
    m_isconnected = false;
    if(isValid()) {
        ::close(m_sock);
        m_sock = -1;
    }
    return true;
}

int Socket::send(const void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::send(const iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

//sendto()函数主要用于SOCK_DGRAM类型套接口向to参数指定端的套接口发送数据报。对于SOCK_STREAM类型套接口，to和tolen参数被忽略
int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}

int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0,  sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recv(void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::recv(iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
    }
    return -1; 
}

int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = buffers;
        msg.msg_iovlen = length;
        msg.msg_name = (void*)from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

Address::ptr Socket::getRemoteAddress() {
    if(m_remoteAddress) {
        return m_remoteAddress;
    }
    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    //获取一个套接口的远端名字
    if(getpeername(m_sock, result->getAddr(), &addrlen)) {
        WUBAI_LOG_ERROR(g_logger) << "getpeername error sock = " << m_sock << " errno = " << errno << " errorstr " << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_remoteAddress = result;
    return m_remoteAddress;
}

Address::ptr Socket::getLocalAddress() {
    if(m_localAddress) {
        return m_localAddress;
    }
    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    //获取一个套接口的本地名字
    if(getsockname(m_sock, result->getAddr(), &addrlen)) {
        WUBAI_LOG_ERROR(g_logger) << "getsockname error sock = " << m_sock << " errno = " << errno << " errorstr " << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_localAddress = result;
    return m_localAddress;
}

bool Socket::isValid() const {
    return m_sock != -1;
}
bool Socket::getError() {
    int error = 0;
    size_t len = sizeof(error);
    if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    return error;
}

std::ostream& Socket::dump(std::ostream& os) const {
    os  << "[Socket sock = " << m_sock
        << " is_connected = " << m_isconnected
        << " family = " << m_family
        << " type = " << m_type
        << " protocol" << m_protocol;
    if(m_localAddress) {
        os << " local_address " << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << " remote_address " << m_remoteAddress->toString();
    }
    os << " ] ";
    return os;
}

std::string Socket::toString() {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

bool Socket::cancelRead() {
    return IOManager::GetThis()->cancelEvent(m_sock, wubai::IOManager::READ);
}

bool Socket::cancelWrite() {
    return IOManager::GetThis()->cancelEvent(m_sock, wubai::IOManager::WRITE);
}

bool Socket::cancelAccept() {
    return IOManager::GetThis()->cancelEvent(m_sock, wubai::IOManager::READ);
}

bool Socket::cancelAll() {
    return IOManager::GetThis()->cancelAll(m_sock);
}

void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if(m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if(WUBAI_LIKELY(m_sock != -1)) {
        initSock();
    } else {
        WUBAI_LOG_ERROR(g_logger) << "socket ( " << m_family << "," << m_type << "," 
        << m_protocol << " ) errno" << errno << " errorstr " << strerror(errno);
    }
}

std::ostream& operator<<(std::ostream& os, const Socket& addr) {
    return addr.dump(os);
}


}