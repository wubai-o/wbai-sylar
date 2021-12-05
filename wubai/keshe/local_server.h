#ifndef __WUBAI_LOCAL_SERVER_H__
#define __WUBAI_LOCAL_SERVER_H__

#include"../socket.h"
#include"../iomanager.h"
#include"../log.h"
#include"../socket_stream.h"
#include"../keshe/mymysql.h"
#include<vector>
#include<memory>
#include<string>
#include<iostream>
#include<sstream>

namespace wubai {
namespace http {

class Localserver : public std::enable_shared_from_this<Localserver> {
public:
typedef std::shared_ptr<Localserver> ptr;

Localserver(IOManager* worker = wubai::IOManager::GetThis(), IOManager* acceptWorker = wubai::IOManager::GetThis());

bool localbind(wubai::Address::ptr addr);
bool nodebind(wubai::Address::ptr addr);

bool start();
void stop() {}

void startAccpet(Socket::ptr sock);
void handleClient(Socket::ptr client);
void accessSQL(std::string str);
void sendmsg(std::string str);
void recvmsg();
void carStart();
void carStop();

private:
    Socket::ptr m_localsock;
    Socket::ptr m_nodesock;
    SocketStream::ptr m_localServer;
    SocketStream::ptr m_nodeServer;
    IOManager* m_worker;
    IOManager* m_acceptWorker;
    Mysql::ptr m_mysql;
};

}
}

#endif