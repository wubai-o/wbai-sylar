#include"../wubai/application.h"

#include<iostream>

int main(int argc, char** argv) {
    wubai::Application app;
    if(app.init(argc, argv)) {
        return app.run();
    }
    return 0;
}