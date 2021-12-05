#include"../wubai/config.h"
#include"../wubai/log.h"
#include"../wubai/env.h"
#include<yaml-cpp/yaml.h>


#if 0
wubai::ConfigVar<int>::ptr g_int_value_config = 
    wubai::Config::Lookup("system.port", (int)8080, "system port");

wubai::ConfigVar<float>::ptr g_int_valuex_config = 
    wubai::Config::Lookup("system.port", (float)8080, "system port");

wubai::ConfigVar<float>::ptr g_float_value_config = 
    wubai::Config::Lookup("system.value", (float)10.2f, "system value");

wubai::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = 
    wubai::Config::Lookup("system.int_vec", std::vector<int>{1,2}, "system int_vect");

wubai::ConfigVar<std::list<int> >::ptr g_int_list_value_config = 
    wubai::Config::Lookup("system.int_list", std::list<int>{1,2}, "system int_list"); 

wubai::ConfigVar<std::set<int> >::ptr g_int_set_value_config = 
    wubai::Config::Lookup("system.int_set", std::set<int>{1,2}, "system int_set"); 

wubai::ConfigVar<std::unordered_set<int> >::ptr g_int_unordered_set_value_config = 
    wubai::Config::Lookup("system.int_unordered_set", std::unordered_set<int>{1,2}, "system int_unordered_set");

wubai::ConfigVar<std::map<std::string, int> >::ptr g_int_map_value_config = 
    wubai::Config::Lookup("system.int_map", std::map<std::string, int>{{"k",2}}, "system int_map");

wubai::ConfigVar<std::unordered_map<std::string, int> >::ptr g_int_unordered_map_value_config = 
    wubai::Config::Lookup("system.int_unordered_map", std::unordered_map<std::string, int>{{"j",2}}, "system int_unordered_map");



void print_yaml(const YAML::Node& node, int level) {            //3-sequence 2-Scalar
    if(node.IsScalar()) {
        WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << "-" << node.Type() << " Scalar";
    } else if(node.IsNull()) {
        WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << "-" << level << " Null";
    } else if(node.IsMap()) {
        for(auto it = node.begin(); it != node.end(); ++it) {
            WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->first.Type() << " - " << level << " Map";
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level << " Sequence";
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("/root/wubai/log.yml");
    print_yaml(root,0);

}

void test_config() {
    WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "before:" << g_int_value_config->getValue();
    WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "before:" << g_float_value_config->toString();

#define XX(g_var, name, prefix) \
{ \
    auto& v = g_var->getValue(); \
    for(auto& i : v) { \
        WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << #prefix " " #name ":" << i; \
    } \
    WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
}

#define XX_M(g_var, name, prefix) \
{ \
    auto& v = g_var->getValue(); \
    for(auto& i : v) { \
        WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << #prefix " " #name ":" << "{ " \
             << i.first << " - " << i.second << " } "; \
    } \
    WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
}

    XX(g_int_vec_value_config, int_vec, before)
    XX(g_int_list_value_config, int_list, before)
    XX(g_int_set_value_config, int_set, before)
    XX(g_int_unordered_set_value_config, int_unordered_set, before)
    XX_M(g_int_map_value_config, int_map, before)
    XX_M(g_int_unordered_map_value_config, int_unordered_map, before)

    YAML::Node root = YAML::LoadFile("/root/wubai/log.yml");
    wubai::Config::LoadFromYaml(root);

    WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "after:" << g_int_value_config->getValue();
    WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "after:" << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after)
    XX(g_int_list_value_config, int_list, after)
    XX(g_int_set_value_config, int_set, after)
    XX(g_int_unordered_set_value_config, int_unordered_set, after)
    XX_M(g_int_map_value_config, int_map, after)
    XX_M(g_int_unordered_map_value_config, int_unordered_map, after)

}

#endif

class Person {
public:
    Person() {}
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "{Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "}";
        return ss.str(); 
    }

    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
               && m_age == oth.m_age
               && m_sex == oth.m_sex;
    }
};

namespace wubai {

template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person,std::string> {
public:
    std::string operator()(const Person& v) {
        YAML::Node node;
        node["name"] = v.m_name;
        node["age"] = v.m_age;
        node["sex"] = v.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
}

wubai::ConfigVar<Person>::ptr g_person = \
    wubai::Config::Lookup("class.person", Person(), "system person");

wubai::ConfigVar<std::map<std::string, Person>>::ptr g_person_map = \
    wubai::Config::Lookup("class.person_map", std::map<std::string, Person>(), "system person map");

wubai::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map = \
    wubai::Config::Lookup("class.person_vec_map", std::map<std::string, std::vector<Person> >(), "class.person_vec_map");

void test_class() {
    WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "before " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
{ \
    auto m = g_var->getValue(); \
    for(auto& i : m) { \
        WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << #prefix << i.first << " - " << i.second.toString(); \
    } \
    WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << #prefix << ":size=" << m.size(); \
}

    g_person->addListener([](const Person& old_value, const Person& new_value) {
        WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "old value = " << old_value.toString() << " new value = " << new_value.toString();
    });

    XX_PM(g_person_map, before);

    WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/root/wubai/log.yml");
    wubai::Config::LoadFromYaml(root);
    
    WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "after" << g_person->getValue().toString() << " - " << g_person->toString();
    //WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "after:" << g_person_map->toString();
    XX_PM(g_person_map, after);
    WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "after" << g_person_vec_map->toString();

}



void test_log() {
    std::cout << wubai::LoggerMgr::GetInstance()->toYamlString();
    std::cout << std::endl;
    YAML::Node root = YAML::LoadFile("/root/wubai/log.yml");
    wubai::Config::LoadFromYaml(root);
    wubai::ConfigVarBase::ptr var = wubai::Config::LookupBase("logs");
    std::cout << var->toString() << std::endl;
    std::cout << "------------------" << std::endl;
    std::cout << wubai::LoggerMgr::GetInstance()->toYamlString();
    WUBAI_LOG_INFO(WUBAI_LOG_NAME("system")) << "old formatter";
    WUBAI_LOG_NAME("system")->setFormatter("%d - %m%n");
    WUBAI_LOG_INFO(WUBAI_LOG_NAME("system")) << "new formatter";
    std::cout << std::endl;
}

void test_loadconf() {
    wubai::Config::LoadFromConfDir("conf");
}

int main(int argc, char** argv) {
    //test_yaml();
    //test_config();
    //test_class();
    //test_log();
    wubai::EnvMgr::GetInstance()->init(argc, argv);
    test_loadconf();
    sleep(10);
    test_loadconf();
    /*
    wubai::Config::Visit([](wubai::ConfigVarBase::ptr var) {
        WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "name = " << var->getName()
            << " description:" << var->getDescription()
            << " typename = " << var->getTypeName()
            << " value:" << var->toString();
    });
    */
    return 0;
}