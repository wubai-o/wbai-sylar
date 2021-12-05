#ifndef __WUBAI_APPLICATION_H__
#define __WUBAI_APPLICATION_H__

#include"env.h"
#include"daemon.h"
#include"tcp_server.h"

namespace wubai {

class Application {
public:
    Application();

    static Application* GetInstance() {return s_instance;}
    bool init(int argc, char** argv);
    bool run();

private:
    int main(int argc, char** argv);
    int run_fiber();
private:
    int m_argc = 0;
    char** m_argv = nullptr;

    std::map<std::string, std::vector<TcpServer::ptr> > m_servers;
    IOManager::ptr m_mainIOManager;
    static Application* s_instance;
};



}   // namespace wubai

#endif // __WUBAI_APPLICATION_H__
