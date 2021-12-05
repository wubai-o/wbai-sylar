#ifndef __WUBAI_HTTP_SERVER_H__
#define __WUBAI_HTTP_SERVER_H__

#include"../tcp_server.h"
#include"http_session.h"
#include"servlet.h"

namespace wubai {
namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer(bool keepalive = false, wubai::IOManager* worker = wubai::IOManager::GetThis(), wubai::IOManager* accept_worker = wubai::IOManager::GetThis());

    ServletDispatch::ptr getServletDispatch() const {return m_dispatch;}
    void setServletDispatch(ServletDispatch::ptr v) {m_dispatch = v;}
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    bool m_isKeepAlive;
    ServletDispatch::ptr m_dispatch;
};








}
}


#endif