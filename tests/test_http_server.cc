#include"../wubai/wubai.h"

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

void run() {
    wubai::http::HttpServer::ptr server(new wubai::http::HttpServer);
    wubai::Address::ptr addr = wubai::Address::LookupAnyIPAddress("0.0.0.0:3333");
    while(!server->bind(addr)) {
        sleep(2);
    }

    auto sd = server->getServletDispatch();
    sd->addServlet("/wubai/xx", [](wubai::http::HttpRequest::ptr req, wubai::http::HttpResponse::ptr rsp, wubai::http::HttpSession::ptr session) {
        rsp->setBody(req->toString());
        return 0;
    });

    sd->addGlobServlet("/wubai/*", [](wubai::http::HttpRequest::ptr req, wubai::http::HttpResponse::ptr rsp, wubai::http::HttpSession::ptr session) {
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });

    server->start();
}


int main(int argc, char** argv) {
    wubai::IOManager iom(2);
    iom.schedule(run);


    return 0;
}