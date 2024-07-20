#include "json.hpp"
#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

using json = nlohmann::json;
typedef struct REGI {
    CHAR *rip;
    CHAR *myip;
    int rport;
    int myport;
} regiInfo;

typedef struct NAME {
    CHAR *rip;
    CHAR *myip;
    int rport;
    int myport;
    CHAR *methodName;
} nameInfo;

void* registName(void* arg) {
    //std::cout<<"In registName"<<std::endl;
    std::unique_ptr<nameInfo> naInfo(reinterpret_cast<nameInfo*>(arg));

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup 失败" << std::endl;
        return nullptr;
    }

    SOCKET nameSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (nameSocket == INVALID_SOCKET) {
        std::cout << "Socket creation 失败： " << WSAGetLastError() << std::endl;
        WSACleanup();
        return nullptr;
    }

    sockaddr_in registerAddr{};
    registerAddr.sin_family = AF_INET;
    registerAddr.sin_port = htons(naInfo->rport);
    inet_pton(AF_INET, naInfo->rip, &registerAddr.sin_addr);
   // std::cout<<"rip:"<<naInfo->rip<<" rport"<<naInfo->rport<<std::endl;
    if (connect(nameSocket, reinterpret_cast<sockaddr*>(&registerAddr), sizeof(registerAddr)) == SOCKET_ERROR) {
        std::cout << "Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(nameSocket);
        WSACleanup();
        return nullptr;
    }

    json nameMess;
    nameMess["type"] = "RegisterName";
    nameMess["ip"] = naInfo->myip;
    nameMess["port"] = naInfo->myport;
    nameMess["method"] = naInfo->methodName;
    std::string name_str = nameMess.dump();
    send(nameSocket, name_str.c_str(), static_cast<int>(name_str.length()), 0);
    std::cout << "注册函数 " << naInfo->methodName << std::endl;

    closesocket(nameSocket);
    WSACleanup();
    return nullptr;
}

void* connection(void* arg) {
    std::unique_ptr<regiInfo> reInfo(reinterpret_cast<regiInfo*>(arg));

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cout << "WSAStartup 失败： " << result << std::endl;
        return nullptr;
    }

    SOCKET heartbeatSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (heartbeatSocket == INVALID_SOCKET) {
        std::cout << "Socket creation 失败： " << WSAGetLastError() << std::endl;
        WSACleanup();
        return nullptr;
    }

    sockaddr_in registerAddr{};
    registerAddr.sin_family = AF_INET;
    registerAddr.sin_port = htons(reInfo->rport);
    inet_pton(AF_INET, reInfo->rip, &registerAddr.sin_addr);

    if (connect(heartbeatSocket, reinterpret_cast<sockaddr*>(&registerAddr), sizeof(registerAddr)) == SOCKET_ERROR) {
        std::cout << "Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(heartbeatSocket);
        WSACleanup();
        return nullptr;
    }

    json registerMess;
    registerMess["type"] = "HeartBeat";
    registerMess["ip"] = reInfo->myip;
    registerMess["port"] = reInfo->myport;
    std::string register_str = registerMess.dump();
    while (true) {
        send(heartbeatSocket, register_str.c_str(), static_cast<int>(register_str.length()), 0);
        std::cout << "send heartbeat" << std::endl;
        Sleep(10000); // 延迟 10 秒
    }
    return nullptr;
}