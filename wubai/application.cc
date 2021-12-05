#include"application.h"
#include"log.h"
#include"config.h"
#include"utils.h"
#include"http/http_server.h"

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

struct HttpServerConf {
    std::vector<std::string> addresses;
    int keepalive = 0;
    int timeout = 2 * 1000 * 60;
    std::string name;

    bool isValid() const {
        return !addresses.empty();
    }

    bool operator==(const HttpServerConf& oth) const {
        return addresses == oth.addresses
            && keepalive == oth.keepalive
            && timeout == oth.timeout
            && name == oth.name;
    }
};

template<>
class LexicalCast<std::string, HttpServerConf> {
public:
    HttpServerConf operator()(const std::string& str) {
        YAML::Node node = YAML::Load(str);
        HttpServerConf conf;
        if(node["address"].IsDefined()) {
            for(size_t i = 0; i < node["address"].size(); ++i) {
                conf.addresses.push_back(node["address"][i].as<std::string>());
            }
        }
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout = node["timeout"].as<int>(conf.timeout);
        conf.name = node["name"].as<std::string>(conf.name);
        return conf;
    }
};

template<>
class LexicalCast<HttpServerConf, std::string> {
public:
    std::string operator()(const HttpServerConf& conf) {
        YAML::Node node;
        node["name"] = conf.name;
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        for(auto& i : conf.addresses) {
            node["address"].push_back(i);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

static wubai::ConfigVar<std::vector<HttpServerConf> >::ptr g_http_server_conf =
    wubai::Config::Lookup("http_servers",
                          std::vector<HttpServerConf>(),
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

    if(!wubai::EnvMgr::GetInstance()->init(argc, argv)) {
        wubai::EnvMgr::GetInstance()->printHelp();
        return false;
    }
    if(wubai::EnvMgr::GetInstance()->has("p")) {
        wubai::EnvMgr::GetInstance()->printHelp();
        return false;
    }

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

    std::string pidfile = g_server_work_path->getValue()
                            + "/" + g_server_pid_file->getValue();
    if(wubai::FSUtil::IsRunningPidfile(pidfile)) {
        WUBAI_LOG_ERROR(g_logger) << "server is running" << pidfile;
        return false;
    }

    std::string conf_path = wubai::EnvMgr::GetInstance()->getAbsolutePath(
        wubai::EnvMgr::GetInstance()->get("c", "conf")
    );

    WUBAI_LOG_INFO(g_logger) << " load conf path:" << conf_path; 
    wubai::Config::LoadFromConfDir(conf_path);

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

int Application::main(int argc, char** argv) {
    std::string pidfile = g_server_work_path->getValue()
                        + "/" + g_server_pid_file->getValue();
    std::ofstream ofs(pidfile);
    if(!ofs) {
        WUBAI_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
        return false;
    }
    // 放入执行逻辑的进程id
    ofs << getpid();

    auto http_confs = g_http_server_conf->getValue();
    for(auto& i : http_confs) {
        WUBAI_LOG_INFO(g_logger) << LexicalCast<HttpServerConf, std::string>()(i);
    }
    wubai::IOManager iom(1);
    iom.schedule(std::bind(&Application::run_fiber, this));
    iom.stop();


    return 0;
}

int Application::run_fiber() {
    auto http_confs = g_http_server_conf->getValue();
    for(auto& conf : http_confs) {
        WUBAI_LOG_INFO(g_logger) << LexicalCast<HttpServerConf, std::string>()(conf);

        std::vector<Address::ptr> addrs;
        std::vector<Address::ptr> fails; 
        for(auto& addr : conf.addresses) {
            size_t pos = addr.find(":");
            if(pos == std::string::npos) {
                WUBAI_LOG_ERROR(g_logger) << "invalid address: " << addr;
                continue;
            }
            auto address = wubai::Address::LookupAnyIPAddress(addr);
            if(address) {
                addrs.push_back(address);
                continue;
            }
            std::vector<std::pair<Address::ptr, uint32_t> > results;
            if(!wubai::Address::GetInterfaceAddresses(results, addr.substr(0, pos))) {
                WUBAI_LOG_ERROR(g_logger) << "invalid address:" << addr;
                continue;
            }
            for(auto& result : results) {
                auto ipaddr = std::dynamic_pointer_cast<IPAddress>(result.first);
                if(ipaddr) {
                    ipaddr->setPort(atoi(addr.substr(pos + 1).c_str()));
                }
                addrs.push_back(ipaddr);
            }
        }
        wubai::http::HttpServer::ptr server(new wubai::http::HttpServer(conf.keepalive));
        if(!server->bind(addrs, fails)) {
            for(auto& fail : fails) {
                WUBAI_LOG_ERROR(g_logger) << "bind address fail:" << *fail;
            }
            exit(0);
        }
        if(!conf.name.empty()) {
            server->setName(conf.name);
        }
        server->start();
    }
    return 0;
}

} // namespace wubai