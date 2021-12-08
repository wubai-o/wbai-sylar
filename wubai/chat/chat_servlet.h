#ifndef __WUBAI_HTTP_CHAT_SERVLET_H__
#define __WUBAI_HTTP_CHAT_SERVLET_H__

#include"../http/ws_servlet.h"
#include<memory>

namespace chat {

class ChatWSServlet : public wubai::http::WSServlet {
public:
    typedef std::shared_ptr<ChatWSServlet> ptr;
    ChatWSServlet();
    virtual int32_t onConnect(wubai::http::HttpRequest::ptr header,
                            wubai::http::WSSession::ptr session) override;

    virtual int32_t onClose(wubai::http::HttpRequest::ptr header,
                            wubai::http::WSSession::ptr session) override;

    virtual int32_t handle(wubai::http::HttpRequest::ptr header,
                        wubai::http::WSFrameMessage::ptr msg,
                        wubai::http::WSSession::ptr session) override;
};


}   // namespace chat



#endif // __WUBAI_HTTP_CHAT_SERVLET_H__