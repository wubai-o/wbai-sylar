#include"resource_servlet.h"
#include"../log.h"

namespace wubai {
namespace http {

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

ResourceServlet::ResourceServlet(const std::string& path) 
    :Servlet("ResourceServlet") 
    ,m_path(path) {}

int32_t ResourceServlet::handle(wubai::http::HttpRequest::ptr request,
                        wubai::http::HttpResponse::ptr response,
                        wubai::http::HttpSession::ptr session) {
    auto path = m_path + "/" + request->getPath();
    WUBAI_LOG_INFO(g_logger) << "handle path=" << path;
    if(path.find("..") != std::string::npos) {
        response->setBody("invalid path");
        response->setStatus(wubai::http::HttpStatus::NOT_FOUND);
        return 0;
    }
    std::ifstream ifs(path);
    if(!ifs) {
        response->setBody("invalid file");
        response->setStatus(wubai::http::HttpStatus::NOT_FOUND);
        return 0;
    }
    std::stringstream ss;
    std::string line;
    while(std::getline(ifs, line)) {
        ss << line << std::endl;
    }
    response->setBody(ss.str());
    response->setHeaders("content-type", "text/html;charset=utf-8");
    return 0;
}
                        

}   // namespace http 
}   // namespace wubai