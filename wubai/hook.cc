#include"hook.h"
#include"fiber.h"
#include"iomanager.h"
#include<dlfcn.h>
#include"fd_manager.h"
#include"config.h"
#include"utils.h"

#include"log.h"

wubai::Logger::ptr g_logger = WUBAI_LOG_NAME("system");

namespace wubai {

static wubai::ConfigVar<int>::ptr g_tcp_connect_timeout = wubai::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;


#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value) {
            WUBAI_LOG_INFO(g_logger) << "tcp connect timeout changed from " << old_value << " to " << new_value;
            s_connect_timeout = new_value;
        });
    }
};

struct _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

struct timer_info {
    int cancelled = 0;
};

template<typename OriginFun, typename ... Args>
static ssize_t do_io(int fd, OriginFun fun,const char* hook_fun_name, uint32_t event, int timeout_so, Args&&... args) {
    //没有开启hook
    if(!wubai::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }
    //没有对应的文件描述符
    wubai::FdCtx::ptr ctx = wubai::FdMgr::GetInstance()->get(fd);
    if(!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }
    //文件描述符是关闭的
    if(ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    //文件描述符不是socket类型的,或者没有设置用户非阻塞
    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    //获得超时时长
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);
    ssize_t n = 0;
    do{
        n = fun(fd, std::forward<Args>(args)...);
        while(n == -1 && errno == EINTR) {
            n = fun(fd, std::forward<Args>(args)...);
        }
        if(n == -1 && errno == EAGAIN) {
            wubai::IOManager* iom = wubai::IOManager::GetThis();
            wubai::Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo);
            if(to != (uint64_t)-1) {
                timer = iom->addConditionTimer(to, [winfo, fd, iom, event](){
                    auto t = winfo.lock();
                    if(!t || t->cancelled) {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, (wubai::IOManager::Event)(event));
                }, winfo);
            }
            int rt = iom->addEvent(fd, (wubai::IOManager::Event)(event));
            if(rt) {
                if(timer) {
                    timer->cancel();
                }
                return -1;
            } else {
                WUBAI_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
                wubai::Fiber::YieldToHold();
                if(timer) {
                    timer->cancel();
                }
                if(tinfo->cancelled) {
                    errno = tinfo->cancelled;
                    return -1;
                }
                continue;
            }
        }
        break;
    }while(true);
    return n;
}


extern "C" {

#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if(!wubai::t_hook_enable) {
        return sleep_f(seconds);
    }
    wubai::Fiber::ptr fiber = wubai::Fiber::GetThis();
    wubai::IOManager* iom = wubai::IOManager::GetThis();
    //iom->addTimer(seconds * 1000, std::bind(&wubai::IOManager::schedule, iom, fiber));
    iom->addTimer(seconds * 1000, [iom, fiber](){
        iom->schedule(fiber);
    });
    wubai::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if(!wubai::t_hook_enable) {
        return usleep_f(usec);
    }
    wubai::Fiber::ptr fiber = wubai::Fiber::GetThis();
    wubai::IOManager* iom = wubai::IOManager::GetThis();
    //iom->addTimer(usec / 1000, std::bind(&wubai::IOManager::schedule, iom, fiber));
    iom->addTimer(usec / 1000, [iom, fiber](){
        iom->schedule(fiber);
    });
    wubai::Fiber::YieldToHold();

    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!wubai::t_hook_enable) {
        return nanosleep_f(req, rem);
    }
    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
        wubai::Fiber::ptr fiber = wubai::Fiber::GetThis();
    wubai::IOManager* iom = wubai::IOManager::GetThis();
    //iom->addTimer(usec / 1000, std::bind(&wubai::IOManager::schedule, iom, fiber));
    iom->addTimer(timeout_ms, [iom, fiber](){
        iom->schedule(fiber);
    });
    wubai::Fiber::YieldToHold();
    return 0;
}


int socket(int domain, int type, int protocol) {
    if(!wubai::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(fd == -1) {
        return fd;
    }
    wubai::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!wubai::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    wubai::FdCtx::ptr ctx = wubai::FdMgr::GetInstance()->get(fd);
    if(!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }
    if(!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }
    if(ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }
    int n = connect_f(fd, addr, addrlen);
    if(n == 0) {
        return 0;
    } else if(n != -1 || errno != EINPROGRESS) {
        return n;
    }
    wubai::IOManager* iom = wubai::IOManager::GetThis();
    wubai::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);
    if(timeout_ms != (uint64_t)-1) {
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom](){
            auto t = winfo.lock();
            if(!t || t->cancelled) {
                return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, wubai::IOManager::WRITE);
        }, winfo);
    }

    int rt = iom->addEvent(fd, wubai::IOManager::WRITE); //connect上就会触发
    if(!rt) {
        wubai::Fiber::YieldToHold();
        if(timer) {
            timer->cancel();
        }
        if(tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        if(timer) {
            timer->cancel();
        }
        WUBAI_LOG_ERROR(g_logger) << "connect addEvent (" << fd << ", WRITE) error";
    }
    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if(!error) {
        return 0;
    } else {
        errno =error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, wubai::s_connect_timeout);
}


int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(sockfd, accept_f, "accept", wubai::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        wubai::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd ,read_f, "read", wubai::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", wubai::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", wubai::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", wubai::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}


ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", wubai::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", wubai::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", wubai::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    return do_io(sockfd, send_f, "send", wubai::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    return do_io(sockfd, sendto_f, "sendto", wubai::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    return do_io(sockfd, sendmsg_f, "sendmsg", wubai::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}


int close(int fd) {
    if(wubai::t_hook_enable) {
        return close_f(fd);
    }
    wubai::FdCtx::ptr ctx = wubai::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = wubai::IOManager::GetThis();
        if(iom) {
            iom->cancelAll(fd);
        }
        wubai::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

//两个与阻塞和非阻塞相关的fcntl函数,F_SETFL F_GETFL F_
int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                wubai::FdCtx::ptr ctx = wubai::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if(ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                wubai::FdCtx::ptr ctx = wubai::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;
                }
                if(ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }

}

//当request是FIONBIO时,与阻塞有关
int ioctl(int d, long unsigned int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg;
        wubai::FdCtx::ptr ctx = wubai::FdMgr::GetInstance()->get(d);
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg); 
}


int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname,const void *optval, socklen_t optlen) {
    if(!wubai::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    //设置超时时间
    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            wubai::FdCtx::ptr ctx = wubai::FdMgr::GetInstance()->get(sockfd);
            if(ctx) {
                const timeval* v = (const timeval*)optval;  //timeval 成员1 tv_sec秒 成员2 tv_usec微秒
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}