#include"http_session.h"
#include"http_parser.h"

namespace wubai {
namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner) 
    :SocketStream(sock, owner){
}

//接收http请求并解析 最后返回HttpRequest
HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    //uint64_t buff_size = 100;
    std::shared_ptr<char> buffer(new char[buff_size], [](char* ptr) {
        delete[] ptr;
    });
    char* data = buffer.get();
    int offset = 0;
    do {
        //rt:读到内容的长度
        int rt = read(data + offset, buff_size - offset);
        if(rt <= 0) {
            close();
            return nullptr;
        }
        //偏移后总长
        rt += offset;
        //解析到内容的长度,并且data偏移nparse
        int nparse = parser->execute(data, rt);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        //外部内容再读取所需偏移量
        offset = rt - nparse;
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    }while(true);
    //剩下的offset属于body的内容
    int64_t length = parser->getContentLength();
    if(length > 0) {
        std::string body;
        body.resize(length);
        int len = 0;
        if(length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= len;
        if(length > 0) {
            if(readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}





}

}