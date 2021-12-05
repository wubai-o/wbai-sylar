#include"../wubai/wubai.h"
#include"../wubai/keshe/local_server.h"


void receiveCycleData() {
    wubai::http::Localserver::ptr server(new wubai::http::Localserver());
    server->start();
}

int main(int argc, char** argv) {
    wubai::IOManager iom(1);
    iom.schedule(receiveCycleData);
    return 0;
}