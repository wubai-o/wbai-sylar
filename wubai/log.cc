#include"log.h"
#include<map>
#include<functional>
#include<iostream>
#include<time.h>
#include<string.h>
#include"config.h"

namespace wubai{

const char* LogLevel::ToString(LogLevel::Level level) {
    switch(level) {
#define XX(name) \
        case LogLevel::name: \
            return #name; \
            break;
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

LogEventWrap::LogEventWrap(LogEvent::ptr e) 
    :m_event(e) {
}
LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}
std::stringstream& LogEventWrap::getSS() {
    return m_event->getSS();
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLogger()->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S") 
        :m_format(format){
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf,sizeof(buf),m_format.c_str(),&tm);
        os<<buf;
    }

private:
    std::string m_format;
};

class FileNameFormatItem : public LogFormatter::FormatItem {
public:
    FileNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {} 
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os<<"\t";
    }
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, \
    int32_t line, uint32_t elapse, uint32_t threadId, uint32_t fiberId, uint64_t time, const std::string& thread_name)
    :m_file(file)
    ,m_line(line)
    ,m_elapse(elapse)
    ,m_threadId(threadId)
    ,m_fiberId(fiberId)
    ,m_time(time)
    ,m_threadName(thread_name)
    ,m_logger(logger)
    ,m_level(level) {
}

void LogEvent::format(const char* fmt, ...) {
    va_list al;
    va_start(al,fmt);
    format(fmt,al);
    va_end(al);
}

void LogEvent::format(const char* fmt,va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf,fmt,al);
    if(len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}


Logger::Logger(const std::string& name) 
    :m_name(name)
    ,m_level(LogLevel::DEBUG) {
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%s%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::addAppender(LogAppender::ptr appender) {
    if(!appender->hasFormatter()) {
        MutexType::Lock alock(appender->m_mutex);
        appender->m_formatter = m_formatter;
    }
    MutexType::Lock lock(m_mutex);
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    for(auto it = m_appenders.begin();it!=m_appenders.end();++it) {
        if(*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders() {
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

void Logger::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
    for(auto& i : m_appenders) {            //当替换logger的formatter格式时,如果输出地没有自己的formatter,替换和logger一样的formatter
        if(!i->hasFormatter()) {
            MutexType::Lock ll(i->m_mutex);
            i->m_formatter = m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string& val) {
    wubai::LogFormatter::ptr new_value(new wubai::LogFormatter(val));
    if(new_value->isError()) {
        std::cout << "Logger set Formatter name= " << m_name 
                  << "value=" << val << "invalid formatter" 
                  << std::endl;
    }
    setFormatter(new_value);
}

LogFormatter::ptr Logger::getFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

void Logger::log(LogLevel::Level level,LogEvent::ptr event) {
    if(level>=m_level) {
        auto self = shared_from_this();
        if(!m_appenders.empty()) {
            MutexType::Lock lock(m_mutex);
            for(auto& i : m_appenders) {
                i->log(self, level, event); 
            }
        }
    }
}

void Logger::debug(LogEvent::ptr event) {
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event) {
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event) {
    log(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event) {
    log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event) {
    log(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string& filename)
    :m_filename(filename) {
        reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level){
        MutexType::Lock lock(m_mutex);
        m_filestream<<m_formatter->format(logger, level, event);
    }
}

std::string FileLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(has && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void LogAppender::setLevel(LogLevel::Level level) {
    m_level = level;
}

bool FileLogAppender::reopen() {
    MutexType::Lock lock(m_mutex);
    if(m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level) {
        MutexType::Lock lock(m_mutex);
        std::cout<<m_formatter->format(logger, level, event);   //return ss.str()
    } 
}

std::string StdoutLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(has && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogFormatter::LogFormatter(const std::string& pattern) 
    :m_pattern(pattern){
        init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for(auto& i : m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

void LogAppender::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
    if(m_formatter){
        has = true;
    } else {
        has = false;
    }
}

std::string Logger::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    node["level"] = LogLevel::ToString(m_level);
    node["formatter"] = m_formatter->getPattern();
    for(auto& a : m_appenders) {
        node["appenders"].push_back(YAML::Load(a->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void LogFormatter::init() {
    //str, format, type
    std::vector<std::tuple<std::string, std::string, int> > vec; //三元组数组
    std::string nstr;
    for (size_t i = 0; i < m_pattern.size(); i++) {
        //不是格式字符,当普通的字符串放入,三元组中的最后一个元素置0
        if (m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }
        // "%%" = % 
        if (i + 1 < m_pattern.size() && m_pattern[i + 1] == '%') {
            nstr.append(1, m_pattern[i]);
            ++i;
            continue;
        }
        size_t n = i + 1;
        size_t fmt_n = 0;
        std::string str;
        std::string fmt = "";
        //当找到
        bool status = false;
        while (n < m_pattern.size()) {
            if (!status && !isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}') {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if (!status && m_pattern[n] == '{') {
                fmt_n = n + 1;
                str = m_pattern.substr(i + 1, n - i - 1);
                ++n;
                status = true;
            }
            if (status && m_pattern[n] == '}') {
                fmt = m_pattern.substr(fmt_n, n - fmt_n);
                ++n;
                status = false;
                break;
            }
            ++n;
            if (n == m_pattern.size() && n - i - 1 == 1) {
                str = m_pattern.substr(i + 1);
                break;
            }
        }
        if (!status) {
            if (!nstr.empty()) {
                vec.push_back(std::make_tuple("", nstr, 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        }
        if (status) {
            return;
        }
    }
    if (!nstr.empty()) {
        vec.push_back(std::make_tuple("", nstr, 0));
    }

    static std::map<std::string,std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
#define XX(str,C) \
        {#str, [](const std::string& fmt){return FormatItem::ptr(new C(fmt));}}
        XX(m,MessageFormatItem),    //%m -- 消息体
        XX(p,LevelFormatItem),      //%p -- level
        XX(r,ElapseFormatItem),     //%r -- 启动后的时间
        XX(c,NameFormatItem),       //%c -- 日志名称
        XX(t,ThreadIdFormatItem),   //%t -- 线程id
        XX(n,NewLineFormatItem),    //%n -- 回车
        XX(d,DateTimeFormatItem),   //%d -- 时间
        XX(f,FileNameFormatItem),   //%f -- 文件名
        XX(l,LineFormatItem),       //%l -- 行号
        XX(T,TabFormatItem),        //%T -- Tab
        XX(F,FiberIdFormatItem),    //%F -- 协程号
        XX(s,ThreadNameFormatItem)  //%s -- 线程名称
#undef XX
    };

    for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            //固定字符
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }
        else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %"+ std::get<0>(i) + ">>")));
                m_error = true;
            }
            else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
        //std::cout << std::get<0>(i) << "-" <<std::get<1>(i) << "-" << std::get<2>(i)<< std::endl;
    }
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    init();
    m_loggers[m_root->m_name] = m_root;
}

std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    std::stringstream ss;
    for(auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    ss << node;
    return ss.str();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) {
        return it->second;
    }
    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

struct LogAppenderDefine {
    int type = 0;   //1 File, 2 Stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type 
        && level == oth.level
        && formatter == oth.formatter
        && file == oth.file;
    }

};

template<>
class LexicalCast<std::string, LogAppenderDefine> {
public:
    wubai::LogAppenderDefine operator()(const std::string& str) {
        YAML::Node node = YAML::Load(str);
        LogAppenderDefine appender;
        std::string type;
        if(node["type"].IsDefined()) {
            type = node["type"].as<std::string>();
            if(type == "FileLogAppender") {
                if(node["file"].IsDefined()) {
                    appender.file = node["file"].as<std::string>();
                    appender.type = 1;
                } else {
                    std::cout << type << std::endl;
                    std::cout << "LogAppenderDefine file error" << std::endl;
                }
            } else if (type == "StdoutLogAppender") {
                    appender.type = 2;
            }
        } else {
            std::cout << "LogAppenderDefine type error" << std::endl;
        }
        if(node["formatter"].IsDefined()) {
            appender.formatter = node["formatter"].as<std::string>();
        }
        return appender;
    }
};

template<>
class LexicalCast<LogAppenderDefine, std::string> {
public:
    std::string operator()(const LogAppenderDefine& v) {
        YAML::Node node;
        std::stringstream ss;
        if(v.type == 1) {
            node["type"] = "FileLogAppender";
            node["file"] = v.file;
        } else if(v.type == 2) {
            node["type"] = "StdoutLogAppender";
        }
        if(v.level!=LogLevel::UNKNOW) {
            node["level"] = LogLevel::ToString(v.level);
        }
        if(!v.formatter.empty()){
            node["formatter"] = v.formatter;
        }
        ss << node;
        return ss.str();
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name 
        && level == oth.level
        && formatter == oth.formatter
        && appenders == oth.appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }
};

template<>
class LexicalCast<std::string, LogDefine> {
public:
    wubai::LogDefine operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        LogDefine logdefine;
        std::stringstream ss;
        logdefine.level = LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");
        if(node["name"].IsDefined()) {
            logdefine.name = node["name"].as<std::string>();    
        }
        if(node["formatter"].IsDefined()) {
            logdefine.formatter = node["formatter"].as<std::string>();
        }
        if(node["appenders"].IsDefined()) {
            ss << node["appenders"];
            logdefine.appenders = LexicalCast<std::string, std::vector<LogAppenderDefine> >()(ss.str());
        }
        return logdefine;
    }
};

template<>
class LexicalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine& v) {
        YAML::Node node;
        std::stringstream ss;
        node["name"] = v.name;
        if(v.level != LogLevel::UNKNOW) {
            node["level"] = LogLevel::ToString(v.level);
        }
        if(v.formatter.empty()) {
                node["level"] = v.formatter;
        }
        node["formatter"] = v.formatter;
        node["appenders"] = YAML::Load(LexicalCast<std::vector<LogAppenderDefine>, std::string>()(v.appenders));
        ss << node;
        return ss.str();
    }
};

std::shared_ptr<wubai::ConfigVar<std::set<LogDefine> > > g_log_defines = 
    wubai::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value){
            WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "on_logger_conf_change";
            for(auto& i : new_value) {
                wubai::Logger::ptr logger;
                logger = WUBAI_LOG_NAME(i.name);
                logger->setLevel(i.level);
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }
                logger->clearAppenders();
                for(auto& a : i.appenders) {
                    wubai::LogAppender::ptr ap;
                    //判断Appender的类型
                    if(a.type == 1) {
                        ap.reset(new wubai::FileLogAppender(a.file));
                    } else if(a.type == 2) {
                        ap.reset(new wubai::StdoutLogAppender());
                    }
                    if(!a.formatter.empty()) {
                        wubai::LogFormatter::ptr fmt(new wubai::LogFormatter(a.formatter));
                        if(!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "appender name = " << a.formatter << "is invalid" << std::endl;
                        }
                    }
                    ap->setLevel(a.level);
                    logger->addAppender(ap);
                }
                for(auto& i : old_value) {
                    auto it = new_value.find(i);
                    if(it == new_value.end()) {
                        //删除logger
                        wubai::Logger::ptr logger = WUBAI_LOG_NAME(i.name);
                        logger->setLevel((LogLevel::Level)100);
                        logger->clearAppenders();
                    }
                }
            }
        });
    }

};

void LoggerManager::init() {
    LogIniter();
}


}
