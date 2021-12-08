#ifndef __WUBAI_HTTP_RESOURCE_SERVLET_H__
#define __WUBAI_HTTP_RESOURCE_SERVLET_H__

#include"../http/servlet.h"

namespace wubai {
namespace http {

class ResourceServlet : public wubai::http::Servlet {
public:
    typedef std::shared_ptr<ResourceServlet> ptr;
    ResourceServlet(const std::string& path);
    virtual int32_t handle(wubai::http::HttpRequest::ptr request,
                        wubai::http::HttpResponse::ptr response,
                        wubai::http::HttpSession::ptr session) override;

private:
    std::string m_path;
};

}
} // namespace wubai





#endif