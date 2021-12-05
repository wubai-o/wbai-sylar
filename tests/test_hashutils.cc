#include"../wubai/hashutils.h"
#include<iostream>


int main(int argc, char** argv) {
    if(argc == 2) {
        std::string code = wubai::base64encode(std::string(argv[1]));
        std::cout << code << std::endl;
        std::cout << wubai::base64decode(code) << std::endl;
    }
    return 0;
}