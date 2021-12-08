#ifndef __WUBAI_TCP_SERVER_H__
#define __WUBAI_TCP_SERVER_H__

#include"iomanager.h"
#include"address.h"
#include"socket.h"
#include"noncopyable.h"
#include"config.h"

#include<memory>
#include<functional>
#include<vector>

namespace wubai {

struct TcpServerConf {
    typedef std::shared_ptr<TcpServerConf> ptr;
    std::vector<std::string> addresses;
    int keepalive = 0;
    int timeout = 2 * 1000 * 60;
    std::string name;
    std::string type = "http";

    bool isValid() const {
        return !addresses.empty();
    }

    bool operator==(const TcpServerConf& oth) const {
        return addresses == oth.addresses
            && keepalive == oth.keepalive
            && timeout == oth.timeout
            && name == oth.name
            && type == oth.type;
    }
};

template<>
class LexicalCast<std::string, TcpServerConf> {
public:
    TcpServerConf operator()(const std::string& str) {
        YAML::Node node = YAML::Load(str);
        TcpServerConf conf;
        if(node["address"].IsDefined()) {
            for(size_t i = 0; i < node["address"].size(); ++i) {
                conf.addresses.push_back(node["address"][i].as<std::string>());
            }
        }
        conf.keepalive = node["keepalive"].as<int>();
        conf.timeout = node["timeout"].as<int>();
        conf.name = node["name"].as<std::string>();
        conf.type = node["type"].as<std::string>();
        return conf;
    }
};

template<>
class LexicalCast<TcpServerConf, std::string> {
public:
    std::string operator()(const TcpServerConf& conf) {
        YAML::Node node;
        node["name"] = conf.name;
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        node["type"] = conf.type;
        for(auto& i : conf.addresses) {
            node["address"].push_back(i);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
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
    std::vector<Socket::ptr> getSocks() const {return m_socks;}

    void setConf(TcpServerConf::ptr v) {m_conf = v;}
    void setConf(const TcpServerConf& v);
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

    TcpServerConf::ptr m_conf;
};




} // namespace wubai











#endif