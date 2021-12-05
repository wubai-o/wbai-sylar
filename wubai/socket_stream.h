#ifndef __WUBAI_SOCKET_STREAM_H__
#define __WUBAI_SOCKET_STREAM_H__

#include<memory>
#include<vector>
#include"stream.h"
#include"socket.h"


namespace wubai {

class SocketStream : public Stream {
public:
    typedef std::shared_ptr<SocketStream> ptr;
    SocketStream(Socket::ptr sock, bool owner = true);
    ~SocketStream();

    virtual int read(void* buff, size_t len) override;
    virtual int read(ByteArray::ptr ba, size_t len) override;

    virtual int write(const void* buff, size_t len) override;
    virtual int write(ByteArray::ptr ba, size_t len) override;

    virtual void close() override;

    Socket::ptr getSocket() const {return m_socket;}
    bool isConnected() const;

private:
    Socket::ptr m_socket;
    bool m_owner;








};









}










#endif