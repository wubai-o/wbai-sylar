#include"fiber.h"
#include"config.h"
#include"macro.h"
#include"scheduler.h"
#include<iostream>

#include<atomic>

namespace wubai {

static Logger::ptr g_logger = WUBAI_LOG_NAME("root");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_t =
    Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 100;
}

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx)) {
        WUBAI_ASSERT_STRING(false,"getcontext");
    }
    ++s_fiber_count;

    WUBAI_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller) 
    :m_id(++s_fiber_id)
    ,m_cb(cb)
    ,use_caller(use_caller) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_t->getValue();
    m_stack = StackAllocator::Alloc(m_stacksize);
    //初始化ucp结构体,将当前上下文保存到ucp中
    if(getcontext(&m_ctx)) {
        WUBAI_ASSERT_STRING(false,"getcontext");
    }
    //修改通过getcontext取得上下文ucp
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    //会调用第二个参数指向的函数
    if(!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
    //WUBAI_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack) {
        WUBAI_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        WUBAI_ASSERT(!m_cb);
        WUBAI_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }

    //WUBAI_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << m_id;
}

void Fiber::reset(std::function<void()> cb) {
    WUBAI_ASSERT(m_stack);
    WUBAI_ASSERT(m_state == TERM || m_state == INIT);
    m_cb = cb;
    if(getcontext(&m_ctx)) {
        WUBAI_ASSERT_STRING(false,"getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        WUBAI_ASSERT_STRING(false, "swapcontext");
    }
}

void Fiber::back() {
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        WUBAI_ASSERT_STRING(false, "swapcontext");
    }
}

//设置当前协程
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

//返回当前协程
Fiber::ptr Fiber::GetThis() {
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    WUBAI_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

//切换到当前协程执行
void Fiber::swapIn() {
    SetThis(this);
    WUBAI_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        WUBAI_ASSERT_STRING(false, "swapcontext");
    }
}

//切换到后台执行
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());

    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        WUBAI_ASSERT_STRING(false, "swapcontext");
    }
}

//设置协程状态
void Fiber::setState(Fiber::Stat state) {
    m_state = state;
    }


//协程切换到后台，并且设置为ready状态
void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

//协程切换到后台，并且设置为hold状态
void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

//总协程数
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();
    WUBAI_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch(std::exception& ex) {
        cur->m_state = EXCEPT;
        WUBAI_LOG_ERROR(WUBAI_LOG_ROOT()) << "Fiber Except: " << ex.what()
            << "fiber id = " << cur->getId()
            << std::endl
            << wubai::BackTraceToString();
    } catch(...) {
        cur->m_state = EXCEPT;
        WUBAI_LOG_ERROR(WUBAI_LOG_ROOT()) << "Fiber Except"
            << "fiber id = " << cur->getId()
            << std::endl
            << wubai::BackTraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();
    WUBAI_ASSERT_STRING(false, "never reach fiber_id = " + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    WUBAI_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch(std::exception& ex) {
        cur->m_state = EXCEPT;
        WUBAI_LOG_ERROR(WUBAI_LOG_ROOT()) << "Fiber Except: " << ex.what()
            << "fiber id = " << cur->getId()
            << std::endl
            << wubai::BackTraceToString();
    } catch(...) {
        cur->m_state = EXCEPT;
        WUBAI_LOG_ERROR(WUBAI_LOG_ROOT()) << "Fiber Except"
            << "fiber id = " << cur->getId()
            << std::endl
            << wubai::BackTraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    WUBAI_ASSERT_STRING(false, "never reach fiber_id = " + std::to_string(raw_ptr->getId()));
}

  



}