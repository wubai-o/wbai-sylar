#include"scheduler.h"
#include"log.h"
#include"macro.h"
#include<iostream>
#include"hook.h"

namespace wubai {

static wubai::Logger::ptr g_logger = WUBAI_LOG_NAME("root");

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) 
    :m_name(name) {
    WUBAI_ASSERT(threads > 0);
    if(use_caller) {
        wubai::Fiber::GetThis();
        --threads;
        WUBAI_ASSERT(GetThis() == nullptr);
        t_scheduler = this;
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        wubai::Thread::SetName(m_name);

        t_fiber = m_rootFiber.get();
        m_rootThreadId = wubai::GetThreadId();
        m_threadIds.push_back(m_rootThreadId);
    } else {
        m_rootThreadId = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    WUBAI_ASSERT(m_stopping);
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if(!m_stopping) {
        return;
    }
    m_stopping = false;
    WUBAI_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();

}

void Scheduler::stop() {
    m_autoStop = true;
    if(m_rootFiber && m_threadCount == 0 
            && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)) {
        WUBAI_LOG_INFO(g_logger) << this << " " << "stopped";
        m_stopping = true;
        if(stopping()) {
            return;
        }
    }


    if(m_rootThreadId != -1) {
        WUBAI_ASSERT(GetThis() == this);
    } else {
        WUBAI_ASSERT(GetThis() != this);
    }
    
    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if(m_rootFiber) {
        tickle();
    }
    
    if(m_rootFiber) {
        //while(!stopping()) {
        //    if(m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::EXCEPT) {
        //        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //        WUBAI_LOG_INFO(g_logger) << "root fiber is term, reset";
        //        t_fiber = m_rootFiber.get();
        //    }
        //    sleep(1);
        //    m_rootFiber->call();
        //}
        if(!stopping()) {
            m_rootFiber->call();
        }
    }

/*
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
*/
    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i]->join();
    }

    if(stopping()) {
        return;
    }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    WUBAI_LOG_INFO(g_logger) << "run";
    wubai::set_hook_enable(true);
    setThis();
    if(wubai::GetThreadId() != m_rootThreadId) {
        t_fiber = Fiber::GetThis().get();
    }
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;
    FiberAndThread ft; 
    while(true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                //这个任务有指定线程,并且不是本线程,不管,遍历完唤醒其他线程
                if(it->thread != -1 && it->thread != wubai::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }
                WUBAI_ASSERT(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }
                ft = *it;
                m_fibers.erase(it++);            
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
        }
        if(tickle_me) {
            tickle();
        }
        if(ft.fiber && ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->setState(Fiber::HOLD);
            }
            ft.reset();
        } else if(ft.cb) {
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::TERM || cb_fiber->getState() == Fiber::EXCEPT) {
                cb_fiber->reset(nullptr);
            } else {
                cb_fiber->setState(Fiber::HOLD);
                cb_fiber.reset();
            }
        } else {
            if(is_active) {
                --m_activeThreadCount;
                std::cout << wubai::GetThreadId() << std::endl;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM) {
                WUBAI_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }
            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->setState(Fiber::HOLD);
            }
        }
    }


}

void Scheduler::tickle() {
    WUBAI_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    WUBAI_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        wubai::Fiber::YieldToHold();
    }
}

}