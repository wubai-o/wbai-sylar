#ifndef __WUBAI_WS_SESSION_H__
#define __WUBAI_WS_SESSION_H__

#include"http_session.h"
#include"../config.h"
#include"../socket.h"
#include"../stream.h"

#include<stdint.h>
#include<memory>

namespace wubai {
namespace http {

#pragma pack(1)
struct WSFrameHead {
    enum OPCODE {
        // 数据分片帧
        CONTINUE = 0,
        // 文本帧
        TEXT_FRAME = 1,
        // 二进制帧
        BIN_FRAME = 2,
        // 断开连接
        CLOSE = 8,
        // PING
        PING = 0x9,
        // PONG
        PONG = 0xA,
    };
    uint32_t opcode: 4;     // 操作代码，决定了应该如何解析后续的数据载荷，如果是不认识的，那么接收端应该断开连接
    bool rsv3: 1;           // 拓展位，一般为0
    bool rsv2: 1;
    bool rsv1: 1;
    bool fin: 1;            // 表示是否是消息的最后一个分片，0不是，1是
    uint32_t payload: 7;    // 数据载荷的大小
    bool mask: 1;           // 掩码位，表示是否对数据载荷进行掩码操作
                            // 从客户端向服务端发送数据时，需要对数据进行掩码操作，反过来不需要。

    std::string toString() const;
};
#pragma pack()

class WSFrameMessage {
public:
    typedef std::shared_ptr<WSFrameMessage> ptr;
    WSFrameMessage(int opcode = 0, const std::string& data = "");

    int getOpcode() const {return m_opcode;}
    void setOpcode(int v) {m_opcode = v;}

    const std::string& getData() const {return m_data;}
    std::string& getData() {return m_data;}
    void setData(const std::string& v) {m_data = v;}

private:
    int m_opcode;
    std::string m_data;
};

class WSSession : public HttpSession {
public:
    typedef std::shared_ptr<WSSession> ptr;
    WSSession(Socket::ptr sock, bool owner = true);

    // server client
    HttpRequest::ptr handleShake();

    WSFrameMessage::ptr recvMessage();
    int32_t sendMessage(WSFrameMessage::ptr msg, bool fin = true);
    int32_t sendMessage(const std::string& msg, int32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);
    int32_t ping();
    int32_t pong();

private:
    bool handleServerShake();
    bool handleClientShake();
};

extern wubai::ConfigVar<uint32_t>::ptr g_websocket_message_max_size;
WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool client);
int32_t WSSendMessage(Stream* stream, WSFrameMessage::ptr msg, bool client, bool fin);
int32_t WSPing(Stream* stream);
int32_t WSPong(Stream* stream);

}   // namespace http
}   // namespace wubai


#endif // __WUBAI_WS_SESSION_H__