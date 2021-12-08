#ifndef __WUBAI_CONFIG_H__
#define __WUBAI_CONFIG_H__

#include"log.h"
#include<boost/lexical_cast.hpp>
#include<yaml-cpp/yaml.h>
#include<list>
#include<vector>
#include<map>
#include<set>
#include<unordered_map>
#include<unordered_set>

#include"thread.h"

namespace wubai {

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name,const std::string& descrption) 
            :m_name(name)
            ,m_descrption(descrption) {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    } 
    virtual ~ConfigVarBase() {}

    const std::string& getName() const {return m_name;}
    const std::string& getDescription() const {return m_descrption;}

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
    virtual std::string getTypeName() const = 0;
protected:
    std::string m_name;
    std::string m_descrption;
};

//F from_type T to_type
template<class F, class T>
class LexicalCast {
public:
    T operator()(const F& v) {
        return boost::lexical_cast<T>(v);
    }
};


//string -> vector 的模板偏特化
template<class T>
class LexicalCast<std::string, std::vector<T> > {
public:
    std::vector<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i = 0;i < node.size(); ++i) {
            //清空stringstream
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

//vector -> string
template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//string -> list
template<class T>
class LexicalCast<std::string, std::list<T> > {
public:
    std::list<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> lst;
        std::stringstream ss;
        for(size_t i = 0;i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            lst.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return lst;
    }
};

//list -> string
template<class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//string -> set
template<class T>
class LexicalCast<std::string, std::set<T> > {
public:
    std::set<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> iset;
        std::stringstream ss;
        for(size_t i = 0;i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            iset.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return iset;
    }
};

//set -> string
template<class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


//string -> unordered_set
template<class T>
class LexicalCast<std::string, std::unordered_set<T> > {
public:
    std::unordered_set<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> iuset;
        std::stringstream ss;
        for(size_t i = 0;i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            iuset.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return iuset;
    }
};

//unordered_set -> string
template<class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//string -> map
template<class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> imap;
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            imap[it->first.Scalar()] = LexicalCast<std::string, T>()(ss.str());
        }
        return imap;
    }
};

//map -> string
template<class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T>& v) {
        YAML::Node node(YAML::NodeType::Map);
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> umap;
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            umap[it->first.Scalar()] = LexicalCast<std::string, T>()(ss.str());
        }
        return umap;
    }
};

template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T>& v) {
        YAML::Node node(YAML::NodeType::Map);
        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};





//fromStr T operator()(const std::string&)
//toString std::string operatot()(const T&)

template<class T, class fromStr = LexicalCast<std::string, T>, class toStr = LexicalCast<T,std::string> >
class ConfigVar : public ConfigVarBase {
public:
    typedef RWMutex RWMutexType;
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void (const T& old_value,const T& new_value)> on_change_cb;


    ConfigVar(const std::string& name, const T& default_value, const std::string& description = "") 
        :ConfigVarBase(name,description)
        ,m_val(default_value) {
    }

    //T -> string
    std::string toString() override {
        try {
            //return boost::lexical_cast<std::string>(m_val);
            RWMutexType::ReadLock lock(m_mutex);
            return toStr()(m_val);
        }catch (std::exception& e) {
            WUBAI_LOG_ERROR(WUBAI_LOG_ROOT()) << "ConfigVar::toString exception" \
                << e.what() << "convert:" << typeid(m_val).name() << "to string";
        }
        return "";
    }
    //string -> T
    bool fromString(const std::string& val) override {
        try {
            //m_val = boost::lexical_cast<T>(val);
            setValue(fromStr()(val));
        }catch (std::exception& e) {
            WUBAI_LOG_ERROR(WUBAI_LOG_ROOT()) << "ConfigVar::fromString exception" \
                << e.what() << "convert:" << typeid(m_val).name();
        }
        return false;
    }

    const T getValue() {  
        RWMutexType::ReadLock lock(m_mutex);
        return m_val;
    }

    void setValue(const T& v) {
        RWMutexType::ReadLock lock(m_mutex);
        if(v == m_val)
            return;
        for(auto& i : m_cbs) {
            i.second(m_val, v);
        }
        lock.unlock();
        RWMutexType::WriteLock ll(m_mutex);
        m_val = v;
    }

    std::string getTypeName() const override {return typeid(T).name();}

    uint64_t addListener(on_change_cb cb) {
        static uint64_t s_fun_id = 0;
        RWMutexType::WriteLock lock(m_mutex);
        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }

    void delListener(uint64_t key) {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }

    void clearListener() {
        m_cbs.clear();
    }

    on_change_cb getListener(uint64_t key) {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second; 
    }
private:
    T m_val;
    //变更回调函数组, uint64_t key要求唯一,一般可以用hash
    std::map<uint64_t, on_change_cb> m_cbs;
    RWMutexType m_mutex;
};


class Config {
public:
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap; 
    typedef RWMutex RWMutexType;
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value, const std::string& description = "") {
        RWMutex::WriteLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it != GetDatas().end()) {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
            if(tmp) {
                WUBAI_LOG_INFO(WUBAI_LOG_ROOT()) << "Lookup name=" << name << "exists";
                return tmp;
            } else {
                WUBAI_LOG_ERROR(WUBAI_LOG_ROOT()) << "Lookup name=" << name << " exists but type not " \
                    << typeid(T).name() << " read type=" << it->second->getTypeName() \
                    << " " << it->second->toString();
                return nullptr;
            }
        }
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678")
                !=std::string::npos) {
            WUBAI_LOG_ERROR(WUBAI_LOG_ROOT()) << "Lookup name invalid:" << name;
            throw std::invalid_argument(name);
        }
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        GetDatas()[name] = v;
        return v;
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        RWMutex::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it == GetDatas().end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);    //基类到派生类的转换
    }
    static void LoadFromYaml(const YAML::Node& root);
    
    static void LoadFromConfDir(const std::string& path, bool force = false);

    static ConfigVarBase::ptr LookupBase(const std::string& name);

    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);  
private:
    static ConfigVarMap& GetDatas() {
        static ConfigVarMap s_datas;
        return s_datas;
    }
    static RWMutexType& GetMutex() {
        static RWMutexType s_mutex;
        return s_mutex;
    }
};


}

#endif
