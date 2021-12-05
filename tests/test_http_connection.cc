#include"../wubai/wubai.h"
#include<iostream>
#include"arpa/inet.h"


wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

void test_pool() {
    wubai::http::HttpConnectionPool::ptr pool(new wubai::http::HttpConnectionPool("www.sylar.top", "", 80, 10, 1000 * 30, 5));
    wubai::IOManager::GetThis()->addTimer(1000, [pool](){
        auto r = pool->doGet("/", 300);
        WUBAI_LOG_INFO(g_logger) << r->toString();
    }, true);
}


void run() {
    wubai::Address::ptr addr = wubai::Address::LookupAnyIPAddress("www.sylar.top:80");
    if(!addr) {
        WUBAI_LOG_INFO(g_logger) << "get addr error";
        return;
    }
    wubai::Socket::ptr sock = wubai::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if(!rt) {
        WUBAI_LOG_INFO(g_logger) << "connect" << *addr << "failed";
        return;
    }
    wubai::http::HttpConnection::ptr conn(new wubai::http::HttpConnection(sock));
    wubai::http::HttpRequest::ptr req(new wubai::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeaders("host", "www.sylar.top");
    WUBAI_LOG_INFO(g_logger) << "req:" << std::endl << *req;
    conn->sendRequest(req);
    auto rsp = conn->recvResponse();
    if(!rsp) {
        WUBAI_LOG_INFO(g_logger) << "recv response failed";
    }
    //WUBAI_LOG_INFO(g_logger) << "rsp:" << std::endl << *rsp;

    WUBAI_LOG_INFO(g_logger) << "==========================";
    auto result = wubai::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300);
    WUBAI_LOG_INFO(g_logger) << "result = " << result->result << " error=" << result->error << " response: " << (result->response ? result->response->toString() : ""); 
    WUBAI_LOG_INFO(g_logger) << "===========================";
    test_pool();
}

int main(int argc, char** argv) {
    wubai::IOManager iom(2);
    iom.schedule(run);
    return 0;
}