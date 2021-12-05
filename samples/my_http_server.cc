#include"../wubai/http/http_server.h"
#include"../wubai/log.h"

wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

void run() {
    wubai::Address::ptr addr = wubai::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        WUBAI_LOG_INFO(g_logger) << "get address error";
        return;
    }

    wubai::http::HttpServer::ptr http_server(new wubai::http::HttpServer());
    http_server->bind(addr);
    http_server->start();
}

int main(int argc, char** argv) {
    wubai::IOManager iom(1);
    iom.schedule(run);
}
