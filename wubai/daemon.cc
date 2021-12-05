#include"daemon.h"
#include"log.h"
#include"config.h"
#include"utils.h"

#include<sys/types.h>
#include<wait.h>
#include<string>
#include<sstream>

namespace wubai {
    
static Logger::ptr g_logger = WUBAI_LOG_ROOT();
static wubai::ConfigVar<int32_t>::ptr g_daemon_restart_interval
    = wubai::Config::Lookup("daemon.restart_interval", (int32_t)5, "daemon restart interval");

static int real_start(int argc, char** argv,
                      std::function<int(int argc, char** argv)> main_cb) {
    return main_cb(argc, argv);
}


static int real_daemon(int argc, char** argv,
                       std::function<int(int argc, char** argv)> main_cb) {
    daemon(1, 0);
    ProcessInfoMgr::GetInstance()->parent_id = getpid();
    ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
    while(true) {
        pid_t pid = fork();
        if(pid == 0) {
            ProcessInfoMgr::GetInstance()->main_id = getpid();
            ProcessInfoMgr::GetInstance()->main_start_time = time(0);
            WUBAI_LOG_INFO(g_logger) << "process start pid = " << getpid();
            return real_start(argc, argv, main_cb);
        } else if(pid < 0) {
            WUBAI_LOG_ERROR(g_logger) << "fork fail return = " << pid <<
                 " errno = " << errno << " errstr = " << strerror(errno);
        } else {
            int status = 0;
            waitpid(pid, &status, 0);
            WUBAI_LOG_INFO(g_logger) << "wait pid: " << pid;
            if(status) {
                WUBAI_LOG_ERROR(g_logger) << "child crash pid = " <<
                    pid << " status = " << status;
            } else {
                WUBAI_LOG_INFO(g_logger) << "finished";
                break;
            }
            ProcessInfoMgr::GetInstance()->restart_count += 1;
            sleep(g_daemon_restart_interval->getValue());
        }
    }
    return 0;
}

int start_daemon(int argc, char** argv,
                 std::function<int(int argc, char** argv)> main_cb,
                 bool is_daemon) {
    if(!is_daemon) {
        return real_start(argc, argv, main_cb);
    }
    return real_daemon(argc, argv, main_cb);
}

std::string ProcessInfo::toString() const {
    std::stringstream ss;
    ss << "[ProcessInfo parent_id=" << parent_id
       << " main_id=" << main_id
       << " parent_start_time=" << formatTime(static_cast<long>(parent_start_time))
       << " main_start_time" << formatTime(static_cast<long>(main_start_time))
       << " restart_count=" << restart_count << "]";
    return ss.str();
}


}