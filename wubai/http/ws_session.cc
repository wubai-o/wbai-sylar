#include"ws_session.h"
#include"../log.h"
#include"../http/http_session.h"
#include"../hashutils.h"
#include"http.h"
#include"../endian.h"

#include<sstream>

namespace wubai {
namespace http {

static wubai::Logger::ptr g_logger = WUBAI_LOG_ROOT();

wubai::ConfigVar<uint32_t>::ptr g_websocket_message_max_size
    = wubai::Config::Lookup("websocket.message.max_size"
        ,(uint32_t) 1024 * 1024 * 32,"websocket message max size");

WSSession::WSSession(Socket::ptr sock, bool owner) 
    :HttpSession(sock, owner) {}

std::string WSFrameHead::toString() const {
    std::stringstream ss;
    ss << "[WSFrameHead fin=" << fin
       << " rsv1=" << rsv1
       << " rsv2=" << rsv2
       << " rsv3=" << rsv3
       << " opcode=" << opcode
       << " mask=" << mask
       << " payload=" << payload
       << "]";
    return ss.str();
}
HttpRequest::ptr WSSession::handleShake() {
    HttpRequest::ptr req;
    do {
        req = recvRequest();
        if(!req) {
            WUBAI_LOG_INFO(g_logger) << "invalid http request";
            break;
        }
        if(strcasecmp(req->getHeader("Upgrade").c_str(), "websocket")) {
            WUBAI_LOG_INFO(g_logger) << "http header Upgrade != websocket";
            break;
        }
        if(strcasecmp(req->getHeader("Connection").c_str(), "Upgrade")) {
            WUBAI_LOG_INFO(g_logger) << "http header Connection != Upgrade";
            break;
        }
        if(req->getHeaderAs<int>("Sec-webSocket-Version") != 13) {
            WUBAI_LOG_INFO(g_logger) << "http header Sec-webSocket-Version != 13";
            break;
        }
        std::string key = req->getHeader("Sec-WebSocket-Key");
        if(key.empty()) {
            WUBAI_LOG_INFO(g_logger) << "http header Sec-WebSocket-Key = null";
            break;
        }

        std::string v = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        v = wubai::base64encode(wubai::sha1sum(v));
        req->setWebsocket(true);

        auto rsp = req->createResponse();
        rsp->setStatus(HttpStatus::SWITCHING_PROTOCOLS);
        rsp->setWebsocket(true);
        rsp->setReason("Web Socket Protocol Handshake");
        rsp->setHeaders("Upgrade", "websocket");
        rsp->setHeaders("Connection", "Upgrade");
        rsp->setHeaders("Sec-WebSocket-Accept", v);
        sendResponse(rsp);
        WUBAI_LOG_INFO(g_logger) << *req;
        WUBAI_LOG_INFO(g_logger) << *rsp;
        return req;
    } while(false);
    if(req) {
        WUBAI_LOG_INFO(g_logger) << *req;
    }
    return nullptr;
}

/*
HttpRequest::ptr WSSession::handleShake() {
    HttpRequest::ptr req;
    do {
        req = recvRequest();
        if(!req) {
            WUBAI_LOG_INFO(g_logger) << "invalid http request";
            break;
        }
        if(strcasecmp(req->getHeader("Upgrade").c_str(), "websocket")) {
            WUBAI_LOG_INFO(g_logger) << "http header Upgrade != websocket";
            break;
        }
        if(strcasecmp(req->getHeader("Connection").c_str(), "Upgrade")) {
            WUBAI_LOG_INFO(g_logger) << "http header Connection != Upgrade";
            break;
        }
        if(req->getHeaderAs<int>("Sec-WebSocket-Version") != 13) {
            WUBAI_LOG_INFO(g_logger) << "http header Sec-WebSocket-Version != 13";
            break;
        }
        std::string key = req->getHeader("Sec-WebSocket-Key");
        if(key.empty()) {
            WUBAI_LOG_INFO(g_logger) << "http header Sec-WebSocket-Key = null";
            break;
        }
        std::string v = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        v = wubai::base64encode(wubai::sha1sum(v));
        req->setWebsocket(true);

        auto rsp = req->createResponse();
        rsp->setStatus(HttpStatus::SWITCHING_PROTOCOLS);
        rsp->setWebsocket(true);
        rsp->setReason("Web Socket Protocol Handshake");
        rsp->setHeaders("Upgrade", "websocket");
        rsp->setHeaders("Connection", "Upgrade");
        rsp->setHeaders("Sec-WebSocket-Accept", v);

        sendResponse(rsp);
        WUBAI_LOG_DEBUG(g_logger) << *req;
        WUBAI_LOG_DEBUG(g_logger) << *rsp;
        return req;
    } while(false);
    if(req) {
        WUBAI_LOG_INFO(g_logger) << *req;
    }
    return nullptr;
}
*/

WSFrameMessage::WSFrameMessage(int opcode, const std::string& data) 
    :m_opcode(opcode)
    ,m_data(data) {}

WSFrameMessage::ptr WSSession::recvMessage() {
    return WSRecvMessage(this, false);
} 

int32_t WSSession::sendMessage(WSFrameMessage::ptr msg, bool fin) {
    return WSSendMessage(this, msg, false, fin);                                                                                                                                                    
}        

int32_t WSSession::sendMessage(const std::string& msg, int32_t opcode, bool fin) {
    return WSSendMessage(this, std::make_shared<WSFrameMessage>(opcode, msg), false, fin);
}

int32_t WSSession::ping() {
    return WSPing(this);
}

int32_t WSSession::pong() {
    return WSPong(this);
}

bool handleServerShake();
bool handleClientShake();

WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool client) {
    int opcode = 0;
    std::string data;
    int cur_len = 0;
    do {
        WSFrameHead ws_head;
        if(stream->readFixSize(&ws_head, sizeof(ws_head)) <= 0) {
            break;
        }
        WUBAI_LOG_DEBUG(g_logger) << "WSFrameHead " << ws_head.toString();
        if(ws_head.opcode == WSFrameHead::PING) {
            WUBAI_LOG_INFO(g_logger) << "PING";
            if(WSPong(stream) <= 0) {
                break;
            }
        } else if(ws_head.opcode == WSFrameHead::PONG) {

        } else if(ws_head.opcode == WSFrameHead::CONTINUE
                || ws_head.opcode == WSFrameHead::TEXT_FRAME
                || ws_head.opcode == WSFrameHead::BIN_FRAME) {
            if(!client && !ws_head.mask) {
                WUBAI_LOG_INFO(g_logger) << "WSFrameHead mask != 1";
                break;
            }
            uint64_t length = 0;
            if(ws_head.payload == 126) {
                uint16_t len = 0;
                if(stream->readFixSize(&len, sizeof(len)) <= 0) {
                    break;
                }
                length = wubai::byteswapOnLittleEndian(len);
            } else if(ws_head.payload == 127) {
                uint64_t len = 0;
                if(stream->readFixSize(&len, sizeof(len)) <= 0) {
                    break;
                }
                length = wubai::byteswapOnLittleEndian(len);
            } else {
                length = ws_head.payload;
            }
            if((cur_len + length) >= g_websocket_message_max_size->getValue()) {
                WUBAI_LOG_WARN(g_logger) << "WSFrameMessage lengt > "
                    << g_websocket_message_max_size->getValue()
                    << " (" << (cur_len + length) << ")";
                break;
            }
            char mask[4] = {0};
            if(ws_head.mask) {
                if(stream->readFixSize(mask, sizeof(mask)) <= 0) {
                    break;
                }
            }
            data.resize(cur_len + length);
            if(stream->readFixSize(&data[cur_len], length) <= 0) {
                break;
            }
            if(ws_head.mask) {
                for(int i = 0; i < (int)length; ++i) {
                    data[cur_len + i] ^= mask[i % 4];
                }
            }
            cur_len += length;
            if(!opcode && ws_head.opcode != WSFrameHead::CONTINUE) {
                opcode = ws_head.opcode;
            }

            if(ws_head.fin) {
                std::ofstream ofs("/root/wubai/bin/html/data");
                ofs << data;
                return WSFrameMessage::ptr(new WSFrameMessage(opcode, std::move(data)));
            }
        } else {
            WUBAI_LOG_DEBUG(g_logger) << "invalid opcode=" << ws_head.opcode;
        }
    } while(true);
    stream->close();
    return nullptr;
}

int32_t WSSendMessage(Stream* stream, WSFrameMessage::ptr msg, bool client, bool fin) {
    do {
        WSFrameHead ws_head;
        memset(&ws_head, 0, sizeof(ws_head));
        ws_head.fin = fin;
        ws_head.opcode = msg->getOpcode();
        ws_head.mask = client;
        uint64_t size = msg->getData().size();
        if(size < 126) {
            ws_head.payload = size;
        } else if(size < 65536) {
            ws_head.payload = 126;
        } else {
            ws_head.payload = 127;
        }

        if(stream->writeFixSize(&ws_head, sizeof(ws_head)) <= 0) {
            break;
        }
        if(ws_head.payload == 126) {
            uint16_t len = size;
            len = wubai::byteswapOnLittleEndian(len);
            if(stream->writeFixSize(&len, sizeof(len)) <= 0) {
                break;
            }
        } else if(ws_head.payload == 127) {
            uint64_t len = size;
            len = wubai::byteswapOnLittleEndian(len);
            if(stream->writeFixSize(&len, sizeof(len)) <= 0) {
                break;
            }
        }
        if(client) {
            char mask[4];
            uint32_t rand_value = rand();
            memcpy(mask, &rand_value, sizeof(mask));
            std::string& data = msg->getData();
            for(size_t i = 0; i < data.size(); ++i) {
                data[i] ^= mask[i % 4];
            }

            if(stream->writeFixSize(mask, sizeof(mask)) <= 0) {
                break;
            }
        }
        if(stream->writeFixSize(msg->getData().c_str(), size) <= 0) {
            break;
        }
        return size + sizeof(ws_head);
    } while(0);
    stream->close();
    return -1;
}

int32_t WSPing(Stream* stream) {
    WSFrameHead ws_head;
    memset(&ws_head, 0, sizeof(ws_head));
    ws_head.fin = 1;
    ws_head.opcode = WSFrameHead::PING;
    int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
    if(v <= 0) {
        stream->close();
    }
    return v;
}

int32_t WSPong(Stream* stream) {
    WSFrameHead ws_head;
    memset(&ws_head, 0, sizeof(ws_head));
    ws_head.fin = 1;
    ws_head.opcode = WSFrameHead::PONG;
    int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
    if(v <= 0) {
        stream->close();
    }
    return v;
}

}   // namespace http
}   // namespace wubai