#include<iostream>
#include"../wubai/log.h"
#include"../wubai/utils.h"

int main() {
    wubai::Logger::ptr logger(new wubai::Logger("logger_wubai"));
    logger->addAppender(wubai::LogAppender::ptr(new wubai::StdoutLogAppender));
    
    wubai::LogAppender::ptr file_appender(new wubai::FileLogAppender("./log.txt"));

    wubai::LogFormatter::ptr fmt(new wubai::LogFormatter("%d%T%p%T%c%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(wubai::LogLevel::ERROR);
    logger->addAppender(file_appender);

    //wubai::LogEvent::ptr event(new wubai::LogEvent(__FILE__, __LINE__, 0, wubai::GetThreadId(), wubai::GetFiberId(), time(0)));
    //logger->log(wubai::LogLevel::DEBUG,event);

    WUBAI_LOG_INFO(logger) << "wubai marco info";
    WUBAI_LOG_ERROR(logger) << "wubai marco error";

    WUBAI_LOG_FMT_ERROR(logger,"test macro fmt error %s\t%d","aa",2);

    auto l = wubai::LoggerMgr::GetInstance()->getLogger("xx");
    WUBAI_LOG_INFO(l) << "xxx";
    return 0;
}