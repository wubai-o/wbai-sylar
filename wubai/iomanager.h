#ifndef __WUBAI_IOMANAGER_H__
#define __WUBAI_IOMANAGER_H__

#include"scheduler.h"
#include"timer.h"
#include"macro.h"
#include"hook.h"

namespace wubai {

class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x4
    };

private:
    struct FdContext {
        typedef Mutex MutexType;
        struct EventContext {
            IOManager* scheduler = nullptr; //事件执行的scheduler
            Fiber::ptr fiber;               //事件协程
            std::function<void()> cb;       //事件的回调函数
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& event_context);
        void triggerEvent(Event event);

        int fd = 0;                 //事件关联的句柄
        EventContext read;      //读事件
        EventContext write;     //写事件
        Event events = NONE;   //已经注册的事件
        MutexType mutex;
    };

public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();

    //0 success,-1 error
    int addEvent(size_t fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(size_t fd, Event event);
    bool cancelEvent(size_t fd, Event event);

    bool cancelAll(size_t fd);

    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void onTimerInsertedAtFront() override;

    void contextsResize(size_t size);

private:
    int m_epfd = 0;
    int m_tickleFds[2];

    std::atomic<size_t> m_pendingEventCount {0};
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;
};

}

#endif