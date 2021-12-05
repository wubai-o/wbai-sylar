#ifndef __WUBAI_LOG_H__
#define __WUBAI_LOG_H__

#include<string>
#include<stdint.h>
#include<memory>
#include<list>
#include<sstream>
#include<fstream>
#include<vector>
#include<thread>
#include<stdarg.h>
#include<map>
#include<exception>

#include"utils.h"
#include"singleton.h"
#include"thread.h"

//使用流式方式将日志级别level的日志写入logger
#define WUBAI_LOG_LEVEL(logger,level) \
    if(logger->getLevel() <= level) \
        wubai::LogEventWrap(wubai::LogEvent::ptr (new wubai::LogEvent(logger, level, __FILE__, __LINE__, 0, wubai::GetThreadId(),\
            wubai::GetFiberId(), time(0), wubai::Thread::GetName()))).getSS()

#define WUBAI_LOG_DEBUG(logger) WUBAI_LOG_LEVEL(logger,wubai::LogLevel::DEBUG)
#define WUBAI_LOG_INFO(logger) WUBAI_LOG_LEVEL(logger,wubai::LogLevel::INFO)
#define WUBAI_LOG_WARN(logger) WUBAI_LOG_LEVEL(logger,wubai::LogLevel::WARN)
#define WUBAI_LOG_ERROR(logger) WUBAI_LOG_LEVEL(logger,wubai::LogLevel::ERROR)
#define WUBAI_LOG_FATAL(logger) WUBAI_LOG_LEVEL(logger,wubai::LogLevel::FATAL)

//使用格式化方式将日志级别level写入logger
#define WUBAI_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        wubai::LogEventWrap(wubai::LogEvent::ptr(new wubai::LogEvent(logger, level, \
                __FILE__, __LINE__, 0, wubai::GetThreadId(),wubai::GetFiberId(), time(0), wubai::Thread::GetName()))).getEvent()->format(fmt,__VA_ARGS__)

#define WUBAI_LOG_FMT_DEBUG(logger, fmt, ...) WUBAI_LOG_FMT_LEVEL(logger, wubai::LogLevel::Level::DEBUG, fmt, __VA_ARGS__)
#define WUBAI_LOG_FMT_INGO(logger, fmt, ...) WUBAI_LOG_FMT_LEVEL(logger, wubai::LogLevel::Level::INFO, fmt, __VA_ARGS__)
#define WUBAI_LOG_FMT_WARN(logger, fmt, ...) WUBAI_LOG_FMT_LEVEL(logger, wubai::LogLevel::Level::WARN, fmt, __VA_ARGS__)
#define WUBAI_LOG_FMT_ERROR(logger, fmt, ...) WUBAI_LOG_FMT_LEVEL(logger, wubai::LogLevel::Level::ERROR, fmt, __VA_ARGS__)
#define WUBAI_LOG_FMT_FATAL(logger, fmt, ...) WUBAI_LOG_FMT_LEVEL(logger, wubai::LogLevel::Level::FATAL, fmt, __VA_ARGS__)

#define WUBAI_LOG_ROOT() wubai::LoggerMgr::GetInstance()->getRoot()
#define WUBAI_LOG_NAME(name) wubai::LoggerMgr::GetInstance()->getLogger(name)



namespace wubai{

class Logger;
class LoggerManager;

//日志级别
class LogLevel{
public:
    enum Level{
        UNKNOW = 0,     
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    static const char* ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& str){
    #define XX(name) \
        if(str == #name) { \
            return LogLevel::name; \
        }
        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
    #undef XX
        return LogLevel::UNKNOW;
    }
};

class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse
            , uint32_t threadId, uint32_t fiberId, uint64_t time,const std::string& thread_name);

    const char* getFile() const {return m_file;}
    int32_t getLine() const {return m_line;}
    uint32_t getElapse() const {return m_elapse;}
    uint32_t getThreadId() const {return m_threadId;}
    uint32_t getFiberId() const {return m_fiberId;}
    uint64_t getTime() const {return m_time;}
    std::shared_ptr<Logger> getLogger() const {return m_logger;}
    LogLevel::Level getLevel() {return m_level;}
    const std::string& getThreadName() const {return m_threadName;}

    std::string getContent() const {return m_ss.str();}
    std::stringstream& getSS() {return m_ss;}

    void format(const char* fmt, ...);
    void format(const char* fmt,va_list al);
private:
    const char* m_file = nullptr;       //文件名
    int32_t m_line = 0;                 //行号
    uint32_t m_elapse = 0;              //程序启动开始到现在的毫秒数
    uint32_t m_threadId = 0;            //线程
    uint32_t m_fiberId = 0;             //协程
    uint64_t m_time = 0;                //时间戳
    std::string m_threadName;           //线程名称
    std::stringstream m_ss;
    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};


class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    std::stringstream& getSS();
    LogEvent::ptr getEvent() const {return m_event;}
private:
    LogEvent::ptr m_event;
};

//日志格式器 
class LogFormatter{
public:
    friend Logger;
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);

    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {}
        virtual void format(std::ostream& os,std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    void init();    //根据给定的m_pattern初始化格式,初始化m_items
    bool isError() const {return m_error;}
    const std::string getPattern() const {return m_pattern;}
private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
    bool m_error = false;
};



//日志输出地
class LogAppender{
friend class Logger;
public:
    typedef SpinMutex MutexType;
    typedef std::shared_ptr<LogAppender> ptr;

    virtual ~LogAppender(){}

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,LogEvent::ptr event) = 0;

    virtual std::string toYamlString() = 0;
    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();
    bool hasFormatter() {return has;}
    LogLevel::Level getLevel() {return m_level;}
    void setLevel(LogLevel::Level level);
    
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    LogFormatter::ptr m_formatter;
    MutexType m_mutex;
    bool has = false;
};

//日志输出器
class Logger : public std::enable_shared_from_this<Logger>{
friend class LoggerManager;
public:
    typedef SpinMutex MutexType;
    typedef std::shared_ptr<Logger> ptr;
    Logger(const std::string& name = "root");

    void log(LogLevel::Level level,LogEvent::ptr event);
    
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();
    void setFormatter(const std::string& val);
    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();

    LogLevel::Level getLevel() const {return m_level;}
    void setLevel(LogLevel::Level val){m_level = val;}

    const std::string& getName() const {return m_name;}

    std::string toYamlString();
private:
    std::string m_name;                     //日志名称      
    LogLevel::Level m_level;                //日志级别
    std::list<LogAppender::ptr> m_appenders;//Appender集合
    MutexType m_mutex;
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
};

//输出到控制台的Appender
class StdoutLogAppender:public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;
};

//输出到文件的Appender
class FileLogAppender:public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;

    //重新打开文件,文件打开成功返回true
    bool reopen();
private:
    std::string m_filename;
    std::ofstream m_filestream;
};

//日志管理器
class LoggerManager {
public:
    typedef SpinMutex MutexType;
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    void init();
    Logger::ptr getRoot() const {return m_root;}
    std::string toYamlString();
private:
    MutexType m_mutex;
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

typedef wubai::Singleton<LoggerManager> LoggerMgr;

}
#endif
