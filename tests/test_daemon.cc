#include"../wubai/daemon.h"
#include"../wubai/iomanager.h"
#include"../wubai/log.h"
#include"../wubai/hook.h"
#include"../wubai/timer.h"

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

wubai::Timer::ptr timer;
int server_main(int argc, char** argv) {
    WUBAI_LOG_INFO(g_logger) << wubai::ProcessInfoMgr::GetInstance()->toString();
    wubai::IOManager iom(1);
    timer = iom.addTimer(1000, [](){
        WUBAI_LOG_INFO(g_logger) << "onTimer";
        static int count = 0;
        if(++count > 10) {
            timer->cancel();
        }
    }, true);
    return 0;
}

int main(int argc, char** argv) {
    return wubai::start_daemon(argc, argv, server_main, argc != 1);
}