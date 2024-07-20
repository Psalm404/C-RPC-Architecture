#include <iostream>
#include <string>
#include <json.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <utility>
#include <cstring>
#pragma comment(lib, "Ws2_32.lib")
using json = nlohmann::json;

class RPCClient {
public:
    RPCClient(const CHAR* h, int p, bool isConnectRegistryCenter) : ip(h), port(p), isConRegistyCenter(isConnectRegistryCenter) {

    };

    ~RPCClient() {
        closesocket(clientSocket);
        WSACleanup();
    }

    template<typename T, typename... Args>
    T call(const std::string& method, Args... args);

    std::pair<const CHAR*, int> callRegistryCenter(const std::string& method);

private:
    void clientConnect(const CHAR* targetip, int targetPort);

    const CHAR* ip;
    int port;
    SOCKET clientSocket{};
    bool isConRegistyCenter;
};

void RPCClient::clientConnect(const CHAR* targetip, int targetPort) {
    std::string target = targetip;
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cout << "WSAStartup failed: " << result << std::endl;
        return;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    // 设置超时选项（这里设置为10秒超时）
    int timeout = 10000;  // 10000 milliseconds
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in targetAddr{};
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(targetPort);
    inet_pton(AF_INET, target.c_str(), &targetAddr.sin_addr);
    //std::cout<<"连接地址"<<target.c_str()<<":"<<targetPort<<std::endl;
    if (connect(clientSocket, (struct sockaddr*)&targetAddr, sizeof(targetAddr)) == SOCKET_ERROR) {
        std::cout << "Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }
}

std::pair<const CHAR*, int> RPCClient::callRegistryCenter(const std::string& method) {
    clientConnect(ip, port);

    json request;
    request["type"] = "ServiceDiscovery";
    request["method"] = method;
    auto request_str = request.dump();
    send(clientSocket, request_str.c_str(), std::strlen(request_str.c_str()), 0);
    std::cout << "请求调用函数" << method << std::endl;

    char buffer[1024] = {0};
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        std::string response_str(buffer, bytesReceived);
        //std::cout << "Received message from registry: " << response_str << std::endl;
        auto json_response = json::parse(response_str);
        if (json_response["result"] == "unfind") {
            std::string resp = "注册中心中没有发现" + method + " ,请检查函数名";
            std::cout << resp << std::endl;
            return std::make_pair(resp.c_str(), -1);
        } else {
            auto serverip = json_response["serverip"].get<std::string>();
            int serverPort = json_response["serverPort"].get<int>();
            closesocket(clientSocket);
            return std::make_pair(serverip.c_str(), serverPort);
        }
    } else {
        std::cout << "Error receiving data or connection closed." << std::endl;
        closesocket(clientSocket);
        return std::make_pair("Error receiving data or connection closed.", -1);
    }
}

template<typename T, typename... Args>
T RPCClient::call(const std::string& method, Args... args) {
    const CHAR* serverip;
    int serverPort;
    if (isConRegistyCenter) {
        std::pair<const CHAR*, int> res = callRegistryCenter(method);
        serverip = res.first;
        serverPort = res.second;
        if (serverPort == -1) {
            return T();
        }
    } else {
        serverip = ip;
        serverPort = port;
    }

    //std::cout << "连接服务端 " << serverip << ":" << serverPort << std::endl;
    clientConnect(serverip, serverPort);

    json request = { {"method", method}, {"params", {args...}} };
    auto request_str = request.dump();
    send(clientSocket, request_str.c_str(), std::strlen(request_str.c_str()), 0);
    //std::cout << "发送消息" << std::endl;

    char buffer[1024] = {0};
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        std::string response_str(buffer, bytesReceived);
        //std::cout << "Received message: " << response_str << std::endl;
        auto json_response = json::parse(response_str);
        return json_response["result"].get<T>();
    } else {
        std::cout << "Error receiving data or connection closed." << std::endl;
        return T();
    }
}