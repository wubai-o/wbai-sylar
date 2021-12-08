#include"chat_servlet.h"
#include"../log.h"
#include"protocol.h"

#include<string>

namespace chat {

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

wubai::RWMutex chat_mutex;

// name -> session
std::map<std::string, wubai::http::WSSession::ptr> chat_sessions;

ChatWSServlet::ChatWSServlet() 
    :WSServlet("ChatWSServlet") {}

bool session_exists(const std::string& id) {
    wubai::RWMutex::ReadLock ock(chat_mutex);
    auto it = chat_sessions.find(id);
    return it != chat_sessions.end();
}


void session_add(const std::string& id, wubai::http::WSSession::ptr session) {
    WUBAI_LOG_INFO(g_logger) << "session_add id=" << id;
    wubai::RWMutex::WriteLock lock(chat_mutex);
    chat_sessions[id] = session;
}

void session_del(const std::string& id) {
    WUBAI_LOG_INFO(g_logger) << "session_add id=" << id;
    wubai::RWMutex::WriteLock lock(chat_mutex);
    chat_sessions.erase(id);
}

int32_t ChatWSServlet::onConnect(wubai::http::HttpRequest::ptr header,
                            wubai::http::WSSession::ptr session) {
    WUBAI_LOG_INFO(g_logger) << "onConnect" << session;
    return 0;
}

int32_t ChatWSServlet::onClose(wubai::http::HttpRequest::ptr header,
                            wubai::http::WSSession::ptr session) {
    auto id = header->getHeader("$id");
    WUBAI_LOG_INFO(g_logger) << "onClose" << session;
    if(!id.empty()) {
        session_del(id);
    }
    return 0;
}

int32_t SendMessage(wubai::http::WSSession::ptr session,
                    ChatMessage::ptr msg) {
    return session->sendMessage(msg->toString()) > 0 ? 0 : 1;
}

int32_t ChatWSServlet::handle(wubai::http::HttpRequest::ptr header,
                        wubai::http::WSFrameMessage::ptr msg,
                        wubai::http::WSSession::ptr session) {
    WUBAI_LOG_INFO(g_logger) << "handle" << session << " - " 
        << " opcode=" << msg->getOpcode()
        << " data=" << msg->getData();

    auto chat_msg = ChatMessage::Create(msg->getData());
    auto id = header->getHeader("$id");
    // 关闭连接
    if(!msg) {
        if(!id.empty()) {
            wubai::RWMutex::WriteLock lock(chat_mutex);
            session_del(id);
        }
        return 1;
    }

    ChatMessage::ptr rsp(new ChatMessage); 
    auto type = chat_msg->get("type");
    if(type == "login_request") {    
        auto name = chat_msg->get("name");
        rsp->set("type", "login_response");
        if(name.empty()) {
            rsp->set("result", "400");
            rsp->set("msg", "name is null");
            return SendMessage(session, rsp);
        } 
        if(!id.empty()) {
            rsp->set("result", "401");
            rsp->set("msg", "logined");
            return SendMessage(session, rsp);
        }
        if(session_exists(name)) {
            rsp->set("result", "402");
            rsp->set("msg", "name exists");
            return SendMessage(session, rsp);
        }
        header->setHeaders("$id", name);
        rsp->set("result", "200");
        rsp->set("msg", "ok");
        session_add(name, session);
        return SendMessage(session, rsp);
    } else if(type == "send_request") {
        rsp->set("type", "send_response");
        auto rsp_msg = chat_msg->get("msg");
        if(rsp_msg.empty()) {
            rsp->set("result", "500");
            rsp->set("msg", "msg is null");
            return SendMessage(session, rsp);
        }

        rsp->set("result", "200");
        rsp->set("msg", "ok");
        return SendMessage(session, rsp);
    }
    return 0;
}




}   // namespace chat