#include"../wubai/wubai.h"

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

void test_fiber() {
    WUBAI_LOG_INFO(WUBAI_LOG_NAME("system")) << "test in fiber";
}

int main(int argc, char** argv) {
    WUBAI_LOG_INFO(g_logger) << "main";
    YAML::Node node = YAML::LoadFile("/root/wubai/log.yml");
    wubai::Config::LoadFromYaml(node);
    wubai::Scheduler sc(2, false, "test");
    sc.start();
    sc.schedule(&test_fiber);
    sc.stop();
    WUBAI_LOG_INFO(g_logger) << "over";
    return 0;
}