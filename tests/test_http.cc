#include"../wubai/wubai.h"


void test_request() {
    wubai::http::HttpRequest::ptr req(new wubai::http::HttpRequest);
    req->setHeaders("host", "www.sylar.top");
    req->setBody("hello wubai");

    req->dump(std::cout) << std::endl;
}

void test_response() {
    wubai::http::HttpResponse::ptr rsp(new wubai::http::HttpResponse);
    rsp->setHeaders("X-X", "wubai");
    rsp->setBody("hello wubai");
    rsp->setStatus((wubai::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}