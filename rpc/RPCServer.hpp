#include "RequestHandler.hpp"
#include "RemoteRegiConn.hpp"
#include <thread>

class RPCServer {
public:
    RPCServer(CHAR *h, int p) : ip(h), port(p), isRemoteConn(false) {};
    void remoteRegist(CHAR *rip, int rport, CHAR *name);
    void connectRemoteRegi(CHAR* rip, int rport);

    template<typename Func>
    void regi(const std::string &name, Func func) {
        // 本地注册
        LocalRegister::funcMap[name] = [func](const json &params) -> json {
            LocalRegister reg;
            return reg.callFunction(func, params);
        };
        // 远程注册到服务中心
        if (isRemoteConn){
            if (rip && rport) {
                remoteRegist(rip, rport, const_cast<CHAR *>(name.c_str()));
            }
        }
    }

    void start() {
        startConn(ip, port);
    }

private:
    CHAR *ip;
    int port;
    CHAR *rip;
    int rport;
    bool isRemoteConn;
};

void RPCServer::remoteRegist(CHAR *rip, int rport, CHAR *name) {
    auto *naInfo = new nameInfo{rip, ip, rport, port, name};
    naInfo->methodName = name;
    std::thread t(registName, naInfo);
    t.detach();
}

void RPCServer::connectRemoteRegi(CHAR *rip, int rport) {
    this->rip = rip;
    this->rport = rport;
    auto *reInfo = new regiInfo{rip, ip, rport, port};
    reInfo->rip = rip;
    reInfo->rport = rport;
    isRemoteConn = true;
    std::thread t(connection, reInfo); // 创建线程发送心跳包
    t.detach();
}