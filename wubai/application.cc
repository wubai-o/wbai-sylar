#include"application.h"
#include"log.h"
#include"config.h"
#include"utils.h"
#include"http/http_server.h"
#include"module.h"
#include"http/ws_server.h"
#include"tcp_server.h"

#include<functional>
#include<sstream>
#include<unistd.h>

namespace wubai {

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

static wubai::ConfigVar<std::string>::ptr g_server_work_path = 
    wubai::Config::Lookup("server.work_path",
                          std::string("/root/wubai/bin"),
                          "server work path");

static wubai::ConfigVar<std::string>::ptr g_server_pid_file = 
    wubai::Config::Lookup("server.pid_file",
                          std::string("wubai.pid"),
                          "server pid file");

static wubai::ConfigVar<std::vector<TcpServerConf> >::ptr g_http_server_conf =
    wubai::Config::Lookup("http_servers",
                          std::vector<TcpServerConf>(),
                          "http server config");

Application* Application::s_instance = nullptr;

Application::Application() {
    s_instance = this;
}

bool Application::init(int argc, char** argv) {
    m_argc = argc;
    m_argv = argv;

    wubai::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    wubai::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    wubai::EnvMgr::GetInstance()->addHelp("c", "conf path default: ./conf");
    wubai::EnvMgr::GetInstance()->addHelp("p", "print help");

    bool is_print_help = false;
    if(!wubai::EnvMgr::GetInstance()->init(argc, argv)) {
        is_print_help = true;
    }
    if(wubai::EnvMgr::GetInstance()->has("p")) {
        is_print_help = true;
    }

    std::string conf_path = wubai::EnvMgr::GetInstance()->getConfigPath();
    WUBAI_LOG_INFO(g_logger) << "load conf path:" << conf_path;
    wubai::Config::LoadFromConfDir(conf_path, true);

    ModuleMgr::GetInstance()->init();
    std::vector<Module::ptr> modules;
    ModuleMgr::GetInstance()->listAll(modules);

    for(auto i : modules) {
        i->onBeforeArgsParse(argc, argv);
    }

    if(is_print_help) {
        wubai::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    for(auto i : modules) {
        i->onAfterArgsParse(argc, argv);
    }
    modules.clear();

    int run_type = 0;
    if(wubai::EnvMgr::GetInstance()->has("s")) {
        run_type = 1;
    }
    if(wubai::EnvMgr::GetInstance()->has("d")) {
        run_type = 2;
    }
    if(run_type == 0) {
        wubai::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    // 检查是否重复启动服务
    std::string pidfile = g_server_work_path->getValue()
                            + "/" + g_server_pid_file->getValue();
    if(wubai::FSUtil::IsRunningPidfile(pidfile)) {
        WUBAI_LOG_ERROR(g_logger) << "server is running" << pidfile;
        return false;
    }

    // 新建工作文件夹
    if(!wubai::FSUtil::Mkdir(g_server_work_path->getValue())) {
        WUBAI_LOG_FATAL(g_logger) << "create work path [" << g_server_work_path->getValue()
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Application::run() {
    bool is_daemon = wubai::EnvMgr::GetInstance()->has("d");
    return start_daemon(m_argc, m_argv, std::bind(&Application::main, this, std::placeholders::_1, std::placeholders::_2), is_daemon);
}

bool Application::getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs) {
    auto it = m_servers.find(type);
    if(it == m_servers.end()) {
        return false;
    }
    svrs = it->second;
    return true;
}

void Application::listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers) {
    m_servers = servers;
}

int Application::main(int argc, char** argv) {
    WUBAI_LOG_INFO(g_logger) << "main";
    std::string pidfile = g_server_work_path->getValue()
                        + "/" + g_server_pid_file->getValue();
    WUBAI_LOG_INFO(g_logger) << "pidfile=" << pidfile;
    std::ofstream ofs(pidfile);
    if(!ofs) {
        WUBAI_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
        return false;
    }
    // 放入执行逻辑的进程id
    ofs << getpid() << std::endl;

    auto http_confs = g_http_server_conf->getValue();
    for(auto& i : http_confs) {
        WUBAI_LOG_INFO(g_logger) << LexicalCast<TcpServerConf, std::string>()(i);
    }
    wubai::IOManager iom(1);
    iom.schedule(std::bind(&Application::run_fiber, this));
    iom.stop();


    return 0;
}

int Application::run_fiber() {
    std::vector<Module::ptr> modules;
    ModuleMgr::GetInstance()->listAll(modules);
    bool has_error = false;
    for(auto& i : modules) {
        if(!i->onLoad()) {
            WUBAI_LOG_ERROR(g_logger) << "module name="
                << i->getName() << " version=" << i->getVersion()
                << " filename=" << i->getFilename();
            has_error = true;
        }
    }

    if(has_error) {
        _exit(0);
    }

    auto http_confs = g_http_server_conf->getValue();
    std::vector<TcpServer::ptr> svrs;
    for(auto& conf : http_confs) {
        WUBAI_LOG_INFO(g_logger) << LexicalCast<TcpServerConf, std::string>()(conf);

        std::vector<Address::ptr> addrs;
        for(auto& addr : conf.addresses) {
            size_t pos = addr.find(":");
            if(pos == std::string::npos) {
                // addrs.push_back(UnixAddress::ptr(new UnixAddress(addr)));
                continue;
            }
            int32_t port = atoi(addr.substr(pos + 1).c_str());
            auto address = wubai::IPAddress::Create(addr.substr(0, pos).c_str(), port);
            if(address) {
                addrs.push_back(address);
                continue;
            }
            std::vector<std::pair<Address::ptr, uint32_t> > results;
            if(wubai::Address::GetInterfaceAddresses(results, addr.substr(0, pos))) {
                for(auto& result : results) {
                    auto ipaddr = std::dynamic_pointer_cast<IPAddress>(result.first);
                    if(ipaddr) {
                        ipaddr->setPort(atoi(addr.substr(pos + 1).c_str()));
                    }
                    addrs.push_back(ipaddr);
                }
                continue;
            }
            auto anyaddr = wubai::Address::LookupAny(addr);
            if(anyaddr) {
                addrs.push_back(anyaddr);
                continue;
            }
            WUBAI_LOG_ERROR(g_logger) << "invalid address: " << addr;
            _exit(0);
        }

        IOManager* accept_worker = wubai::IOManager::GetThis();
        IOManager* worker = wubai::IOManager::GetThis();

        TcpServer::ptr server;
        if(conf.type == "http") {
            server.reset(new wubai::http::HttpServer(conf.keepalive, worker, accept_worker));
        } else if(conf.type == "ws") {
            server.reset(new wubai::http::WSServer(worker, accept_worker));
        } else {
            WUBAI_LOG_ERROR(g_logger) << "invalid server type=" << conf.type
                << LexicalCast<TcpServerConf, std::string>()(conf);
            _exit(0);
        }

        if(!conf.name.empty()) {
            server->setName(conf.name);
        }

        std::vector<Address::ptr> fails;
        if(!server->bind(addrs, fails)) {
            for(auto& fail : fails) {
                WUBAI_LOG_ERROR(g_logger) << "bind address fail:" << *fail;
            }
            _exit(0);
        }

        server->setConf(conf);
        m_servers[conf.type].push_back(server);

        svrs.push_back(server);
    }

    for(auto& i : modules) {
        i->onServerReady();
    }

    for(auto& i : svrs) {
        i->start();
    }

    for(auto& i : modules) {
        i->onServerUp();
    }
    return 0;
}

} // namespace wubai