#ifndef __WUBAI_TCP_SERVER_H__
#define __WUBAI_TCP_SERVER_H__

#include"iomanager.h"
#include"address.h"
#include"socket.h"
#include"noncopyable.h"

#include<memory>
#include<functional>
#include<vector>

namespace wubai {

struct TcpServerConf {
    typedef std::shared_ptr<TcpServerConf> ptr;
    
    std::vector<std::string> addrs;
    int keepalive = 0;
    int timeout = 1000 * 2 * 60;
    int ssl = 0;
    std::string id;
    std::string type = "http";
    // ...
};



class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;
    TcpServer(wubai::IOManager* worker = wubai::IOManager::GetThis(), wubai::IOManager* accecpt_worker = wubai::IOManager::GetThis());
    virtual ~TcpServer();

    virtual bool bind(wubai::Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fail_addrs);
    virtual bool start();
    virtual void stop();

    uint64_t getRecvTimeout() const {return m_recvTimeout;}
    std::string getName() const {return m_name;}
    void setRecvTimeout(uint64_t time) {m_recvTimeout = time;}
    void setName(const std::string& name) {m_name = name;}
    bool isStop() const {return m_isStop;}

protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccpet(Socket::ptr sock);

protected:
    std::vector<Socket::ptr> m_socks;
    IOManager* m_worker;
    IOManager* m_acceptWorker;
    uint64_t m_recvTimeout;
    std::string m_name;
    std::string m_type = "tcp";
    bool m_isStop;
};










}











#endif