#ifndef __WUBAI_DAEMON_H__
#define __WUBAI_DAEMON_H__

#include<functional>
#include<unistd.h>
#include"singleton.h"

namespace wubai {

struct ProcessInfo {
    pid_t parent_id = 0;
    pid_t main_id = 0;
    uint64_t parent_start_time = 0;
    uint64_t main_start_time = 0;
    uint64_t restart_count = 0;

    std::string toString() const;
};

typedef Singleton<ProcessInfo> ProcessInfoMgr;

int start_daemon(int argc, char** argv,
                 std::function<int(int argc, char** argv)> main_cb,
                 bool is_daemon);



}   // namespace wubai

#endif