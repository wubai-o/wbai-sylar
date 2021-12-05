#include"../wubai/wubai.h"
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<sys/epoll.h>



wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();


void test_fiber1() {
    WUBAI_LOG_INFO(g_logger) << "test_fiber1";
}

void test_fiber() {
    WUBAI_LOG_INFO(g_logger) << "test_fiber";
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;       
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "14.215.177.38", &addr.sin_addr.s_addr);

    if(!connect(sock, (sockaddr*)&addr, sizeof(addr))) {

    } else if(errno == EINPROGRESS) {
        WUBAI_LOG_INFO(g_logger) << "add event errno = " << errno << " " << strerror(errno);
        wubai::IOManager::GetThis()->addEvent(sock, wubai::IOManager::READ, [](){
            WUBAI_LOG_INFO(g_logger) << "read callback";
        });
        wubai::IOManager::GetThis()->addEvent(sock, wubai::IOManager::WRITE, [sock](){
            WUBAI_LOG_INFO(g_logger) << "write callback";
            wubai::IOManager::GetThis()->cancelEvent(sock, wubai::IOManager::READ);
            close(sock);
        });
    } else {
        WUBAI_LOG_INFO(g_logger) << "else" << errno << strerror(errno);
    }
}

void test1() {
    wubai::IOManager iom;
    iom.schedule(&test_fiber);
}

wubai::Timer::ptr s_timer;
void test_timer() {
    wubai::IOManager iom(2);
    s_timer = iom.addTimer(500, [](){
        WUBAI_LOG_INFO(g_logger) << "hello timer";
        static int i = 0;
        if(++i == 5) {
            s_timer->reset(1000, true);
            //s_timer->cancel();
        }
    }, true);
}


int main(int argc, char** argv) {
    YAML::Node node = YAML::LoadFile("/root/wubai/log.yml");
    wubai::Config::LoadFromYaml(node);
    //test1();
    test_timer();
    return 0;
}