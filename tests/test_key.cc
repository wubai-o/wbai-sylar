#include"../wubai/http/ws_session.h"
#include"../wubai/log.h"
#include"../wubai/http/http_session.h"
#include"../wubai/hashutils.h"
#include"../wubai/http/http.h"
#include"../wubai/endian.h"

#include<iostream>

int main() {
    std::string n = "w4v7O6xFTi36lq3RNcgctw==";
    std::string v = n + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    v = wubai::base64encode(wubai::sha1sum(v));
    std::cout << v << std::endl;
    return 0;
}