#ifndef __WUBAI_UTILS_H__
#define __WUBAI_UTILS_H__

#include<pthread.h>
#include<sys/types.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<stdint.h>
#include<execinfo.h>
#include<sys/time.h> 
#include<vector>


namespace wubai {

pid_t GetThreadId();
uint64_t GetFiberId();

void BackTrace(std::vector<std::string>& bt, int size, int skip);
std::string BackTraceToString(int size = 100, int skip = 2, const std::string& prefix = "");

//时间ms
uint64_t GetCurrentMs();
uint64_t GetCurrentUs();

std::string formatTime(time_t time, const std::string& format = "%Y-%m-%d %H:%M:%S");

class FSUtil {
public:
static bool ListAllFiles(std::vector<std::string>& files,
                         const std::string& path,
                         const std::string& subfix);

static bool IsRunningPidfile(const std::string& pidfile);

static bool Mkdir(const std::string& dirname);
};

} // namespace wubai
#endif