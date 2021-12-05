#include"http_server.h"
#include"../log.h"
#include<iostream>

namespace wubai {
namespace http {

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

HttpServer::HttpServer(bool keepalive, wubai::IOManager* worker, wubai::IOManager* accept_worker) 
    :TcpServer(worker, accept_worker)
    ,m_isKeepAlive(keepalive){
    m_dispatch.reset(new ServletDispatch);
    m_type = "http";
}

void HttpServer::handleClient(Socket::ptr client) {
    wubai::http::HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest();
        if(!req) {
            WUBAI_LOG_WARN(g_logger) << "recv http request fail, errno = " << errno << " error str = " 
                << strerror(errno) << " client = " << *client;
            break;
        }
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClosed() || !m_isKeepAlive));
        m_dispatch->handle(req, rsp, session);
        //rsp->setBody("hello wubai");
        //WUBAI_LOG_INFO(g_logger) << "request:" << std::endl << *req;
        //WUBAI_LOG_INFO(g_logger) << "response:" << std::endl << *rsp;
        session->sendResponse(rsp);
    } while(m_isKeepAlive);
    session->close();
}


}
}