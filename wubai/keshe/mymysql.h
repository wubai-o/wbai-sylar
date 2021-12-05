#ifndef __WUBAI_KESHE_MYMYSQL_H__
#define __WUBAI_KESHE_MYMYSQL_H__

#include"/usr/include/mysql/mysql.h"
#include<string>
#include<sstream>
#include<vector>
#include<iostream>
#include<memory>

namespace wubai {


class Mysql {

public:
typedef std::shared_ptr<Mysql> ptr;

Mysql(const char* host, const char* user, const char* password, const char* db, unsigned int port, const char* uinx_socket = NULL) 
    :m_host(host)
    ,m_user(user)
    ,m_password(password)
    ,m_db(db)
    ,m_port(port)
    ,m_uinx_socket(uinx_socket){
    mysql_init(&m_mysql);
    connect();
}

bool connect();
void close();
void addData(std::string str);

private:
    MYSQL m_mysql;
    const char* m_host;
    const char* m_user;
    const char* m_password;
    const char* m_db;
    unsigned int m_port;
    const char* m_uinx_socket;
    unsigned long m_client_flag = 0;
};









}









#endif