#include"log.h"
#include"utils.h"
#include"fiber.h"

#include<dirent.h>
#include<unistd.h>
#include<string.h>
#include<iostream>
#include<sys/types.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<ifaddrs.h>

namespace wubai {

static wubai::Logger::ptr g_logger = WUBAI_LOG_NAME("system");

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint64_t GetFiberId() {
    return Fiber::GetFiberId();
}

void BackTrace(std::vector<std::string>& bt, int size, int skip) {
    void** array = (void**)malloc((sizeof(void*) * size));
    size_t s = ::backtrace(array, size);

    char** strings = backtrace_symbols(array, s);
    if(strings == NULL) {
        WUBAI_LOG_ERROR(g_logger) << "backtrace_symbols error";
        return;
    }

    for(size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }
    free(array);
    free(strings);
}

std::string BackTraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt;
    BackTrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}

uint64_t GetCurrentMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

uint64_t GetCurrentUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

std::string formatTime(time_t time, const std::string& format) {
    struct tm tm;
    char buf[64];
    localtime_r(&time, &tm);
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}

int8_t TypeUtil::ToChar(const std::string& str) {
    if(str.empty()) {
        return 0;
    }
    return *str.begin();
}

int64_t TypeUtil::Atoi(const std::string& str) {
    if(str.empty()) {
        return 0;
    }
    return strtoull(str.c_str(), nullptr, 10);
}

double TypeUtil::Atof(const std::string& str) {
    if(str.empty()) {
        return 0;
    }
    return atof(str.c_str());
}

int8_t TypeUtil::ToChar(const char* str) {
    if(str == nullptr) {
        return 0;
    }
    return str[0];
}

int64_t TypeUtil::Atoi(const char* str) {
    if(str == nullptr) {
        return 0;
    }
    return strtoull(str, nullptr, 0);   
}
double TypeUtil::Atof(const char* str) {
    if(str == nullptr) {
        return 0;
    }
    return atof(str);
}


in_addr_t GetIPv4Inet() {
    struct ifaddrs* ifas = nullptr;
    struct ifaddrs* ifa = nullptr;

    in_addr_t localhost = inet_addr("127.0.0.1");
    if(getifaddrs(&ifas)) {
        WUBAI_LOG_ERROR(g_logger) << "getifaddrs errno=" << errno
            << " errorstr=" << strerror(errno);
        return localhost;
    }

    in_addr_t ipv4 = localhost;
    for(ifa = ifas; ifa && ifa->ifa_addr; ifa = ifa->ifa_next) {
        if(ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        if(!strncasecmp(ifa->ifa_name, "lo", 2)) {
            continue;
        }
        ipv4 = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr)->sin_addr.s_addr;
        if(ipv4 == localhost) {
            continue;
        }
    }
    if(ifas != nullptr) {
        freeifaddrs(ifas);
    }
    return ipv4;
}

std::string _GetIPv4() {
    std::shared_ptr<char> ipv4(new char[INET_ADDRSTRLEN], wubai::delete_array<char>);
    memset(ipv4.get(), 0, INET_ADDRSTRLEN);
    auto ia = GetIPv4Inet();
    inet_ntop(AF_INET, &ia, ipv4.get(), INET_ADDRSTRLEN);
    return ipv4.get();
}

std::string GetIPv4() {
    static const std::string ip = _GetIPv4();
    return ip;
}

static int __lstat(const char* file, struct stat* st = nullptr) {
    struct stat lst;
    int ret = lstat(file, &lst);
    if(st) {
        *st = lst;
    }
    return ret;
}

static int __mkdir(const char* dirname) {
    if(access(dirname, F_OK) == 0) {
        return 0;
    }
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

bool FSUtil::ListAllFiles(std::vector<std::string>& files,
                          const std::string& path,
                          const std::string& subfix) {
    if(access(path.c_str(), 0) != 0) {
        return false;
    }
    DIR* dir = opendir(path.c_str());
    if(dir == nullptr) {
        return false;
    }
    struct dirent* dp = nullptr;
    while((dp = readdir(dir)) != nullptr) {
        if(dp->d_type == DT_DIR) {
            if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
                continue;
            }
            ListAllFiles(files, path + "/" + dp->d_name, subfix);
        } else if(dp->d_type == DT_REG) {
            std::string filename(dp->d_name);
            if(subfix.empty()) {
                files.push_back(path + "/" + filename);
            } else {
                if(filename.size() < subfix.size()) {
                    continue;
                }
                if(filename.substr(filename.length() - subfix.size()) == subfix) {
                    files.push_back(path + "/" + filename);
                }
            }
        }
    }
    closedir(dir);
    return true;
}

bool FSUtil::IsRunningPidfile(const std::string& pidfile) {
    struct stat st;
    if(lstat(pidfile.c_str(), &st) != 0) {
        return false;    
    }
    std::ifstream ifs(pidfile);
    std::string line;
    if(!ifs || !std::getline(ifs, line)) {
        return false;
    }
    if(line.empty()) {
        return false;
    }
    pid_t pid = atoi(line.c_str());
    if(pid <= 1) {
        return false;
    }
    if(kill(pid, 0) != 0) {
        // 检测进程是否运行
        return false;
    }
    return true;
}

bool FSUtil::Mkdir(const std::string& dirname) {
    if(__lstat(dirname.c_str()) == 0) {
        return true;
    }
    char* path = strdup(dirname.c_str());
    char* ptr = strchr(path + 1, '/');
    do {
        for(; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {
            *ptr = '\0';
            if(__mkdir(path) != 0) {
                break;
            }
        }
        if(ptr != nullptr) {
            break;
        } else if(__mkdir(path) != 0) {
            break;
        }
        free(path);
        return true;
    } while(0);
    free(path);
    return false;
}

}   // namespace wubai