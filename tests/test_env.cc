#include"../wubai/env.h"
#include<iostream>
#include<fstream>

struct A {
    A() {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());
        for(size_t i = 0; i < content.size(); ++i) {
            std::cout << i << "-" << content[i] << "-" << static_cast<int>(content[i]) << std::endl;
        }
    }
};

A a;

int main(int argc, char** argv) {
    wubai::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    wubai::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    wubai::EnvMgr::GetInstance()->addHelp("p", "print help");
    if(!wubai::EnvMgr::GetInstance()->init(argc, argv)) {
        wubai::EnvMgr::GetInstance()->printHelp();
        return 0;
    }
    std::cout << "exe=" << wubai::EnvMgr::GetInstance()->getExe()
              << " cwd=" << wubai::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << wubai::EnvMgr::GetInstance()->getEnv("PATH", "xxx") << std::endl;
    std::cout << "TEST=" << wubai::EnvMgr::GetInstance()->getEnv("TEST", "xxx") << std::endl;
    wubai::EnvMgr::GetInstance()->setEnv("TEST", "yy");
    std::cout << "TEST=" << wubai::EnvMgr::GetInstance()->getEnv("TEST", "xxx") << std::endl;


    if(wubai::EnvMgr::GetInstance()->has("p")) {
        wubai::EnvMgr::GetInstance()->printHelp();
    }
    return 0;
}