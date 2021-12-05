#ifndef __WUBAI_HTTP_WS_SERVLET_H__
#define __WUBAI_HTTP_WS_SERVLET_H__

#include"servlet.h"
#include"ws_session.h"

#include<memory>
#include<string>

namespace wubai {
namespace http {

class WSServlet : public Servlet {
public:
    typedef std::shared_ptr<WSServlet> ptr;
    WSServlet(const std::string& name)
        :Servlet(name) {}

    virtual ~WSServlet() {}

    virtual int32_t handle(wubai::http::HttpRequest::ptr request,
                        wubai::http::HttpResponse::ptr response,
                        wubai::http::HttpSession::ptr session) override {
        return 0;
    }

    virtual int32_t onConnect(wubai::http::HttpRequest::ptr header,
                        wubai::http::WSSession::ptr session) = 0;
    virtual int32_t onClose(wubai::http::HttpRequest::ptr header,
                        wubai::http::WSSession::ptr session) = 0;
    virtual int32_t handle(wubai::http::HttpRequest::ptr header,
                        wubai::http::WSFrameMessage::ptr msg,
                        wubai::http::WSSession::ptr session) = 0;

    const std::string& getName() const {return m_name;}
protected:
    std::string m_name;
};

class FunctionWSServlet : public WSServlet {
public:
    typedef std::shared_ptr<FunctionWSServlet> ptr;
    typedef std::function<int32_t(wubai::http::HttpRequest::ptr,
                                wubai::http::WSSession::ptr)> on_connect_cb;
    typedef std::function<int32_t(wubai::http::HttpRequest::ptr,
                                wubai::http::WSSession::ptr)> on_close_cb;
    typedef std::function<int32_t(wubai::http::HttpRequest::ptr,
                                wubai::http::WSFrameMessage::ptr,
                                wubai::http::WSSession::ptr)> callback;

    FunctionWSServlet(callback cb,
                    on_connect_cb connect_cb = nullptr,
                    on_close_cb close_cb = nullptr);
    

    virtual int32_t onConnect(wubai::http::HttpRequest::ptr header,
                            wubai::http::WSSession::ptr session) override;
    virtual int32_t onClose(wubai::http::HttpRequest::ptr header,
                            wubai::http::WSSession::ptr session) override;
    virtual int32_t handle(wubai::http::HttpRequest::ptr header,
                            wubai::http::WSFrameMessage::ptr msg,
                            wubai::http::WSSession::ptr session) override;
protected:
    callback m_callback;
    on_connect_cb m_onConnect;
    on_close_cb m_onClose;
};

class WSServletDispatch : public ServletDispatch {
public:
    typedef std::shared_ptr<WSServletDispatch> ptr;
    typedef RWMutex RWMutexType;

    WSServletDispatch();
    void addServlet(const std::string& uri,
                    FunctionWSServlet::callback cb,
                    FunctionWSServlet::on_connect_cb connect_cb = nullptr,
                    FunctionWSServlet::on_close_cb close_cb = nullptr);

    void addGlobServlet(const std::string& uri,
                    FunctionWSServlet::callback cb,
                    FunctionWSServlet::on_connect_cb connect_cb = nullptr,
                    FunctionWSServlet::on_close_cb close_cb = nullptr);
    WSServlet::ptr getWSServlet(const std::string& uri);
};


} // namespace http
} // namespace wubai


#endif // __WUBAI_HTTP_WS_SERVLET_H__