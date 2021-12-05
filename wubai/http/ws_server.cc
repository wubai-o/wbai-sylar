#include"ws_server.h"

namespace wubai {
namespace http {

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

WSServer::WSServer(wubai::IOManager* worker,
             wubai::IOManager* accept_worker) 
    :TcpServer(worker, accept_worker) {
    m_dispatch.reset(new WSServletDispatch);
    m_type = "websocket_server";
}

void WSServer::handleClient(Socket::ptr client) {
    WUBAI_LOG_INFO(g_logger) << "handleClient" << *client;
    WSSession::ptr session(new WSSession(client));
    do {
        HttpRequest::ptr header = session->handleShake();
        if(!header) {
            WUBAI_LOG_DEBUG(g_logger) << "handleShake error";
            break;
        }
        WSServlet::ptr servlet = m_dispatch->getWSServlet(header->getPath());
        if(!servlet) {
            WUBAI_LOG_DEBUG(g_logger) << "no match WSServlet";
            break;
        }
        int rt = servlet->onConnect(header, session);
        if(rt) {
            WUBAI_LOG_DEBUG(g_logger) << "onConnect return " << rt;
            break;
        }
        while(true) {
            auto msg = session->recvMessage();
            if(!msg) {
                break;
            }
            rt = servlet->handle(header, msg, session);
            if(rt) {
                WUBAI_LOG_DEBUG(g_logger) << "handle return " << rt;
                break;
            }
        }
        servlet->onClose(header, session);
    } while(0);
    session->close();
}

} // namespace http
} // namespace wubai