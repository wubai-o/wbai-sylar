#ifndef __WUBAI_WS_SERVER_H__
#define __WUBAI_WS_SERVER_H__

#include"../tcp_server.h"
#include"ws_session.h"
#include"ws_servlet.h"

namespace wubai {
namespace http {

class WSServer : public TcpServer {
public:
    typedef std::shared_ptr<WSServer> ptr;

    WSServer(wubai::IOManager* worker = wubai::IOManager::GetThis(),
             wubai::IOManager* accept_worker = wubai::IOManager::GetThis());
    
    WSServletDispatch::ptr getWSServletDispatch() const {return m_dispatch;}
    void setWSServletDispatch(WSServletDispatch::ptr v) {m_dispatch = v;}
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    WSServletDispatch::ptr m_dispatch;
};


} // namespace http
} // namespace wubai

#endif