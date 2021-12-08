#include"../module.h"

namespace chat {

class MyModule : public wubai::Module {
public:
    typedef std::shared_ptr<MyModule> ptr;
    MyModule();
    bool onLoad() override;
    bool onUnLoad() override;
    bool onServerReady() override;
    bool onServerUp() override;
};

}