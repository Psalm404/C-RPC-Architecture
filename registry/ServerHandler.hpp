#include "json.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <random>

using json = nlohmann::json;

// 定义 ServerInfo 结构体
struct ServerInfo {
    std::string ip;
    int port;
    std::chrono::steady_clock::time_point lastHeartbeat;

    // 构造列表
    ServerInfo(const std::string& h, int p) : ip(h), port(p), lastHeartbeat(std::chrono::steady_clock::now()) {}

    // 重载等于运算符
    bool operator==(const ServerInfo& other) const {
        return (ip == other.ip) && (port == other.port);
    }
};


std::unordered_map<std::string, ServerInfo> serverMap;
std::unordered_map<std::string, std::vector<ServerInfo>> methodMap;
std::condition_variable cv;
std::mutex serverMapMutex;
int loadBalance(size_t size){
    srand((unsigned int)time(NULL));
    int index = rand() % size;
    return index;
}
void sendMessage(SOCKET clientSocket, std::string ip, int port){

    if (ip == "unfind"){
        json response;
        response["result"] = "unfind";
        std::string resp_str = response.dump();
        send(clientSocket, resp_str.c_str(), std::strlen(resp_str.c_str()), 0);
    }else{
        json response;
        response["result"] = "find";
        response["serverip"] = ip;
        response["serverPort"] = port;
        std::string resp_str = response.dump();
        send(clientSocket, resp_str.c_str(), std::strlen(resp_str.c_str()), 0);
    }
}

void serviceDiscover(SOCKET clientSocket,  const std::string &method){
   // std::cout<<"in serviceDiscover"<<std::endl;
    std::string ip;
    int port;
    auto it = methodMap.find(method);
    if (it != methodMap.end()) {
        std::vector<ServerInfo> *serverList;
        serverList = &it->second;
        auto size = (*serverList).size();
        try{
            int index = loadBalance(size);
            ServerInfo info = (*serverList)[index];
            ip = info.ip;
            port = info.port;
        }catch (std::exception &e){
            std::cout<<"出错了"<<std::endl;
        }
        sendMessage(clientSocket, ip, port);

    }else{
        sendMessage(clientSocket, "unfind", 0);
    }

}
// 处理服务器函数
void serverHandler(const std::string& ip, int port) {
    ServerInfo info{ip, port};
    {
        std::lock_guard<std::mutex> lock(serverMapMutex);
        auto it = serverMap.find(ip);
        if (it != serverMap.end()) {
            //std::cout<<"update a server"<<std::endl;
            it->second.lastHeartbeat = std::chrono::steady_clock::now();
        } else {
            serverMap.emplace(ip, info);
            std::cout<<"添加一个服务提供者"<<std::endl;
        }
    }
    cv.notify_all();
}

void updateMethod(const std::string& ip, int port, const std::string& methodName){
    ServerInfo info{ip, port};
    auto it = methodMap.find(methodName);
    if (it != methodMap.end()){
        std::vector<ServerInfo>* serverList;
        serverList = &it->second;
        (*serverList).push_back(info);
       // std::cout<<"the method "<<methodName<<" have already existed, the listSize = "<<(*serverList).size()<<std::endl;
    }else{
        std::vector<ServerInfo> serverList;
        serverList.push_back(info);
        methodMap.emplace(methodName, serverList);
       // std::cout<<"create a method list"<<methodName<<std::endl;
    }
}

void checkTimeouts() {
    const double timeoutDuration = 15; // 设置超时时间为15秒
    while (true) {
        {
            std::unique_lock<std::mutex> lock(serverMapMutex);
            cv.wait(lock, [] { return !serverMap.empty(); });
        }

        auto now = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(serverMapMutex);
            for (auto it = serverMap.begin(); it != serverMap.end();) {
                auto duration = now - it->second.lastHeartbeat;
                double duration_sec = std::chrono::duration<double>(duration).count();
              //  std::cout<<"duration:"<<duration_sec<<std::endl;
                if (duration_sec > timeoutDuration) {
                    std::cout<<"去除一个超时的服务提供者"<<std::endl;
                    ServerInfo deadServer = it->second;
                    it = serverMap.erase(it);
                    for (auto mt = methodMap.begin(); mt != methodMap.end();){
                        std::vector<ServerInfo>* serverList;
                        serverList = &mt->second;
                        auto deadServerIt = std::find((*serverList).begin(), (*serverList).end(), deadServer);
                        if (deadServerIt != (*serverList).end()){
                            (*serverList).erase(deadServerIt);
                            if ((*serverList).empty()){
                                mt = methodMap.erase(mt);
                            }else{++mt;}
                            //std::cout<<"a dead method has been erased and the size is "<<(*serverList).size()<<std::endl;
                        }
                    }
                } else {
                    ++it;
                }
            }
        }

       // std::cout << "Checked timeouts. Active servers: " << serverMap.size() << std::endl;
    }
}