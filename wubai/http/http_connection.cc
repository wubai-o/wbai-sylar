#include"http_connection.h"
#include"http_parser.h"
#include"../log.h"
#include<iostream>
#include<sstream>

namespace wubai {
namespace http {

wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "[HttpResult result = " << result
       << " error = " << error
       << " response = " << (response ? response->toString() : "nullptr")
       << " ]";
    return ss.str();
}

HttpResult::ptr HttpConnection::DoGet(const std::string& urlstr, uint64_t timeout_ms,
                                    const std::map<std::string, std::string>& headers, const std::string& body) {
    Uri::ptr uri = Uri::Create(urlstr);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url:" + urlstr);
    }
    return DoGet(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri, uint64_t timeout_ms,
                                    const std::map<std::string, std::string>& headers, const std::string& body) {
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(const std::string& urlstr, uint64_t timeout_ms,
                                    const std::map<std::string, std::string>& headers, const std::string& body) {
    Uri::ptr uri = Uri::Create(urlstr);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url:" + urlstr);
    }
    return DoPost(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri, uint64_t timeout_ms,
                                    const std::map<std::string, std::string>& headers, const std::string& body) {
    return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
                                    const std::map<std::string, std::string>& headers, const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(method);
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") ==  0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
            continue;
        }
        req->setHeaders(i.first, i.second);
    }
    if(!has_host) {
        req->setHeaders("host", uri->getHost());
    }
    req->setBody(body);
    return DoRequest(req, uri, timeout_ms);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, const std::string& urlstr, uint64_t timeout_ms,
                                    const std::map<std::string, std::string>& headers, const std::string& body) {
    Uri::ptr uri = Uri::Create(urlstr);
    return DoRequest(method, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timeout_ms) {
    Address::ptr addr = uri->createAddress();
    if(!addr) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST, nullptr, "invalid host: " + uri->getHost());
    }
    Socket::ptr sock = Socket::CreateTCP(addr);
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_FAIL, nullptr, "connected failed" + addr->toString());
    }
    if(!sock->connect(addr)) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL, nullptr, "connect socket fail" + addr->toString());
    }
    sock->setRecvTimeout(timeout_ms);
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    int rt = conn->sendRequest(req);

    if(rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "send close by peer" + addr->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr,
                                             "send socket error , errno = " + std::to_string(errno) + " errorstr: " + std::string(strerror(errno)));
    }

    auto rsp = conn->recvResponse();


    if(!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr,
                                             "recv response timeout " + addr->toString() + "timeout_ms: " + std::to_string(timeout_ms));
    }

    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}


HttpConnection::HttpConnection(Socket::ptr sock, bool owner) 
    :SocketStream(sock, owner) {
}

HttpConnection::~HttpConnection() {
    WUBAI_LOG_DEBUG(g_logger) << "HttpConnection::~HttpConnection";   
}


//接收http响应并解析 最后返回HttpRequest
HttpResponse::ptr HttpConnection::recvResponse() {
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize();
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
        //这次读到的内容加上上一次未解析的内容长度之和
        rt += offset;
        data[rt] = '\0';
        //解析内容,并将data偏移到解析内容之后
        int nparse = parser->execute(data, rt, false);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        //剩余并未解析的长度
        offset = rt - nparse;
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    }while(true);
    auto& client_parser = parser->getParser();
    std::string body;
    if(client_parser.chunked) {
        int len = offset;   //offset剩余读到但并未解析的长度
        do {
            do {
                int rt2 = read(data + len, buff_size - len);
                if(rt2 <= 0) {
                    close();
                    return nullptr;
                }
                len += rt2;
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, true);
                if(parser->hasError()) {
                    close();
                    return nullptr;
                }
                len -= nparse;
                if(len == (int)buff_size) {
                    close();
                    return nullptr;
                }
            } while(!parser->isFinished());

            //实际剩余的 大于 规定的
            if(client_parser.content_len + 2 <= len) {
                body.append(data, client_parser.content_len);
                memmove(data, data + client_parser.content_len + 2, len - client_parser.content_len - 2);
                len -= client_parser.content_len + 2;
            } else {
            //实际剩余的 小于 规定的
            //还要读数据,直到这份body接收完
                body.append(data, len);
                //memmove(data, data + len, client_parser.content_len - len);
                //left还需读到body中的数据
                int left = client_parser.content_len - len + 2;
                while(left > 0) {
                    int rt = read(data, left > (int)buff_size ? buff_size : left);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    body.append(data, rt);
                    //memmove(data, data + rt, left - rt);
                    left -= rt;
                }
                body.resize(body.size() - 2);
                len = 0;
            }
        } while(!client_parser.chunks_done);
        parser->getData()->setBody(body);
    } else {
        int64_t length = parser->getContentLength();
        if(length > 0) {
            body.resize(length);
            int len = 0;
            if(length >= offset) {
                memcpy(&body[0], data, offset);
                len = offset;
            } else {
                memcpy(&body[0], data, length);
                len = length;
            }
            length -= offset;
            if(length > 0) {
                if(readFixSize(&body[len], length) <= 0) {
                    close();
                    return nullptr;
                }
            }
            parser->getData()->setBody(body);
        }
    }
    return parser->getData();
}

int HttpConnection::sendRequest(HttpRequest::ptr req) {
    std::stringstream ss;
    ss << *req;
    std::string data = ss.str();
    std::cout << ss.str() << std::endl;
    return writeFixSize(data.c_str(), data.size());
}

HttpConnectionPool::HttpConnectionPool(const std::string& host, const std::string& vhost, uint32_t port, uint32_t maxSize, uint32_t max_alive_time, uint32_t max_request) 
    :m_host(host)
    ,m_vhost(vhost)
    ,m_port(port)
    ,m_maxSize(maxSize)
    ,m_maxAliveTime(max_alive_time)
    ,m_maxRequest(max_request) {

}

HttpConnection::ptr HttpConnectionPool::getConnection() {
    uint64_t now_ms = wubai::GetCurrentMs();
    //存放失效连接
    std::vector<HttpConnection*> invalid_conns;
    HttpConnection* ptr = nullptr;
    MutexType::Lock lock(m_mutex);
    while(!m_conns.empty()) {
        auto conn = *m_conns.begin();
        m_conns.pop_front();
        if(!conn->isConnected()) {
            invalid_conns.push_back(conn);
            continue;
        }
        if((conn->m_createTime + m_maxAliveTime) > now_ms) {
            invalid_conns.push_back(conn);
            continue;  
        }
        ptr = conn;
        break;
    }
    lock.unlock();
    for(auto i : invalid_conns) {
        delete i;
    }
    m_total -= invalid_conns.size();
    //池中没有能用的连接 创建一个连接
    if(!ptr) {
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
        std::cout << m_host << std::endl;
        if(!addr) {
            WUBAI_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }
        //设置端口
        addr->setPort(m_port);
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock) {
            WUBAI_LOG_ERROR(g_logger) << "get sock fail:" << *addr;
            return nullptr;
        }
        if(!sock->connect(addr)) {
            WUBAI_LOG_ERROR(g_logger) << "sock connect fail:" << *addr;
        }
        ptr = new HttpConnection(sock);
        ++m_total;
    } 
    return HttpConnection::ptr(ptr, std::bind(HttpConnectionPool::ReleasePtr, std::placeholders::_1, this));
}

HttpResult::ptr HttpConnectionPool::doGet(const std::string& urlstr, uint64_t timeout_ms,
                                     const std::map<std::string, std::string>& headers, const std::string& body) {
    return doRequest(HttpMethod::GET, urlstr, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri, uint64_t timeout_ms,
                                     const std::map<std::string, std::string>& headers, const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?") << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();
    return doGet(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(const std::string& urlstr, uint64_t timeout_ms,
                                     const std::map<std::string, std::string>& headers, const std::string& body) {
    return doRequest(HttpMethod::POST, urlstr, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri, uint64_t timeout_ms,
                                     const std::map<std::string, std::string>& headers, const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "?" : "") << uri->getQuery()
       << (uri->getFragment().empty() ? "#" : "") << uri->getFragment();
    return doPost(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const std::string& urlstr, uint64_t timeout_ms,
                                    const std::map<std::string, std::string>& headers, const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(urlstr);
    req->setMethod(method);
    req->setClose(false);
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") ==  0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
            continue;
        }
        req->setHeaders(i.first, i.second);
    }
    if(!has_host) {
        if(m_vhost.empty()) {
            req->setHeaders("host", m_host);
        } else {
            req->setHeaders("host", m_vhost);
        }
    }
    req->setBody(body);
    return doRequest(req, timeout_ms);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
                                    const std::map<std::string, std::string>& headers, const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?") << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req, uint64_t timeout_ms) {
    auto conn = getConnection();
    if(!conn) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION, nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    auto sock = conn->getSocket();
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONNECTION, nullptr, "pool s");
    }
    sock->setRecvTimeout(timeout_ms);
    int rt = conn->sendRequest(req);
    if(rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "send close by peer" + sock->getRemoteAddress()->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr,
                                             "send socket error , errno = " + std::to_string(errno) + " errorstr: " + std::string(strerror(errno)));
    }
    auto rsp = conn->recvResponse();
    if(!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr,
                                             "recv response timeout " + sock->getRemoteAddress()->toString() + "timeout_ms: " + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}

void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {
    ++ptr->m_request;
    if(!ptr->isConnected() || (ptr->m_createTime + pool->m_maxAliveTime >= wubai::GetCurrentMs()) || ptr->m_request > pool->m_maxRequest) {
        delete ptr;
        --pool->m_total;
        return;
    }
    std::cout << wubai::GetCurrentMs() << " " << ptr->m_createTime << " " << pool->m_maxAliveTime << std::endl;
    std::cout << ptr->m_request << std::endl;
    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);

}

}

}