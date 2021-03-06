#ifndef __WUBAI_CHAT_PROTOCOL_H__
#define __WUBAI_CHAT_PROTOCOL_H__

#include<string>
#include<memory>
#include<map>

namespace chat {

class ChatMessage {
public:
    typedef std::shared_ptr<ChatMessage> ptr;

    static ChatMessage::ptr Create(const std::string& v);
    ChatMessage();
    std::string get(const std::string& name);
    void set(const std::string& name, const std::string& val);
    std::string toString() const;
private:

    std::map<std::string, std::string> m_datas;
};

}


#endif