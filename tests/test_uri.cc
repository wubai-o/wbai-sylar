//#include"../wubai/wubai.h"
#include"../wubai/wubai.h"
#include<iostream>


int main(int argc, char** argv) {
    wubai::Uri::ptr uri = wubai::Uri::Create("http://www.sylar.top/?id=100&name=sylar#frg");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}