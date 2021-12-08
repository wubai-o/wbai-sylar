#include"my_module.h"
#include"../log.h"
#include"../tcp_server.h"
#include"../application.h"
#include"../http/http_server.h"
#include"../http/ws_server.h"
#include"chat_servlet.h"
#include"resource_servlet.h"

namespace chat {

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

MyModule::MyModule() 
    :wubai::Module("chat_room", "1.0", "") {}

bool MyModule::onLoad() {
    WUBAI_LOG_INFO(g_logger) << "onLoad";
    return true;
}

bool MyModule::onUnLoad() {
    WUBAI_LOG_INFO(g_logger) << "onUnLoad";
    return true;
}

bool MyModule::onServerReady() {
    WUBAI_LOG_INFO(g_logger) << "onServerReady";
    std::vector<wubai::TcpServer::ptr> svrs;
    if(!wubai::Application::GetInstance()->getServer("http", svrs)) {
        WUBAI_LOG_INFO(g_logger) << "no httpserver alive";
        return false;
    }

    for(auto& i : svrs) {
        wubai::http::HttpServer::ptr http_server = std::dynamic_pointer_cast<wubai::http::HttpServer>(i);
        if(!i) {
            continue;
        }
        auto slt_dispatch = http_server->getServletDispatch();
        wubai::http::ResourceServlet::ptr slt(new wubai::http::ResourceServlet(wubai::EnvMgr::GetInstance()->getCwd()));
        slt_dispatch->addGlobServlet("/html/*", slt);
    }
    svrs.clear();
    if(!wubai::Application::GetInstance()->getServer("ws", svrs)) {
        WUBAI_LOG_INFO(g_logger) << "no websocket alive";
        return false;
    }
    for(auto& i : svrs) {
        wubai::http::WSServer::ptr ws_server = std::dynamic_pointer_cast<wubai::http::WSServer>(i);
        wubai::http::ServletDispatch::ptr slt_dispatch = ws_server->getWSServletDispatch();
        ChatWSServlet::ptr slt(new ChatWSServlet());
        slt_dispatch->addServlet("/wubai/chat", slt);
    }

    return true;
}

bool MyModule::onServerUp() {
    WUBAI_LOG_INFO(g_logger) << "onServerUp";
    return true;
}

}

extern "C" {

wubai::Module* CreateModule() {
    wubai::Module* module = new chat::MyModule;
    WUBAI_LOG_INFO(chat::g_logger) << "CreateModule" << module;
    return module;
}

void DestroyModule(wubai::Module* module) {
    WUBAI_LOG_INFO(chat::g_logger) << "DestroyModule" << module;
    delete module;
}

} // namespace chat