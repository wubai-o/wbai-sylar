#include"http.h"


namespace wubai {
namespace http {

HttpMethod StringToHttpMethod(const std::string& m) {
#define XX(num, name, string) \
    if(strncmp(#string, m.c_str(), sizeof(m)) == 0) { \
        return HttpMethod::name; \
    }
    HTTP_METHOD_MAP(XX)
#undef XX
    return HttpMethod::INVALID_METHOD;
}

HttpMethod CharsToHttpMethod(const char* m) {
#define XX(num, name, string) \
    if(strncmp(#string, m, strlen(#string)) == 0) { \
        return HttpMethod::name; \
    }
    HTTP_METHOD_MAP(XX)
#undef XX
    return HttpMethod::INVALID_METHOD;
}

static const char* s_method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

const char* HttpMethodToString(const HttpMethod& m) {
    uint32_t idx = uint32_t(m);
    if(idx >=  (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
        return "<unknown>";
    }
    return s_method_string[idx];
}

const char* HttpStatusToString(const HttpStatus& s) {
    switch(s) {
#define XX(num, name, string) \
        case HttpStatus::name: \
            return #string;
        HTTP_STATUS_MAP(XX)
#undef XX
        default:
            return "<unknown>";
    }
}

bool CaseInsensitiveLess::operator()(const std::string &lhs, const std::string &rhs) const {
    //strcasecmp 判断字符串是否相等(忽略大小写)
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}


HttpRequest::HttpRequest(uint8_t version, bool close) 
    :m_method(HttpMethod::GET)
    ,m_version(version)
    ,m_close(close)
    ,m_path("/"){
}

std::shared_ptr<HttpResponse> HttpRequest::createResponse() {
    HttpResponse::ptr rsp(new HttpResponse(getVersion(), isClosed()));
    return rsp;
}

std::string HttpRequest::getHeader(const std::string& key, const std::string& def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

std::string HttpRequest::getParam(const std::string& key, const std::string& def) const {
    auto it = m_param.find(key);
    return it == m_param.end() ? def : it->second;
}

std::string HttpRequest::getCookie(const std::string& key, const std::string& def) const {
    auto it = m_cookie.find(key);
    return it == m_cookie.end() ? def : it->second;
}

void HttpRequest::setHeaders(const std::string& key, const std::string& val) {
    m_headers[key] = val;
}

void HttpRequest::setParam(const std::string& key, const std::string& val) {
    m_param[key] = val;
}

void HttpRequest::setCookie(const std::string& key, const std::string& val) {
    m_cookie[key] = val;
}

void HttpRequest::delHeader(const std::string& key) {
    m_headers.erase(key);
}

void HttpRequest::delParam(const std::string& key) {
    m_param.erase(key);
}

void HttpRequest::delCookie(const std::string& key) {
    m_cookie.erase(key);
}

bool HttpRequest::hasHeader(const std::string& key, std::string* val) {
    auto it = m_headers.find(key);
    if(it == m_headers.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasParam(const std::string& key, std::string* val) {
    auto it = m_param.find(key);
    if(it == m_param.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasCookie(const std::string& key, std::string* val) {
    auto it = m_cookie.find(key);
    if(it == m_cookie.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

std::ostream& HttpRequest::dump(std::ostream& os) const {
    //GET /url HTTP/1.1
    //Hosy: www.baidu.com
    //
    //
    os << HttpMethodToString(m_method) << " "
        << m_path << (m_query.empty() ? "" : "?") << m_query
        << (m_fragment.empty() ? "" : "#") << m_fragment
        << " HTTP/" << ((uint32_t)(m_version >> 4)) << "."
        << ((uint32_t)(m_version & 0x0F)) << "\r\n";

    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    for(auto& i : m_headers) {
        if(strcasecmp(i.first.c_str(), "connect") == 0) {
            continue;
        }
        os << i.first << ":" << i.second << "\r\n";
    }
    if(!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
            << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}

std::string HttpRequest::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

HttpResponse::HttpResponse(uint8_t version, bool close) 
    :m_status(HttpStatus::OK)
    ,m_version(version)
    ,m_close(close) {
}

std::string HttpResponse::getHeaders(const std::string& key, const std::string& def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

void HttpResponse::setHeaders(const std::string& key, const std::string& val) {
    m_headers[key] = val;
}

void HttpResponse::delHeaders(const std::string& key) {
    m_headers.erase(key);
}

std::ostream& HttpResponse::dump(std::ostream& os) const {
    os << "HTTP/"
        << ((uint32_t)(m_version >> 4)) << "."
        << ((uint32_t)(m_version & 0x0f)) << " "
        << (uint32_t)m_status << " "
        << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason) << "\r\n";

    for(auto& i : m_headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }
    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    if(!m_body.empty()) {
        os << "conten-length: " << m_body.size() << "\r\n\r\n" << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}

std::string HttpResponse::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& req) {
    return req.dump(os);
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp) {
    return rsp.dump(os);
}

}


}