#include "json.hpp"
#include <ws2tcpip.h>
#include <winsock2.h>
#include <cstring>
#include <thread>
#include "ServerHandler.hpp"
#pragma comment(lib, "Ws2_32.lib")

using json = nlohmann::json;

SOCKET registerSocket;

void requestDispatcher(int receivedSocket, char buffer[1024], int bytesReceived);
void handleClient(SOCKET clientSocket);

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        getpeername(clientSocket, (sockaddr *) &clientAddr, &clientAddrSize);
        char clientip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientip, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddr.sin_port);

        buffer[bytesReceived] = '\0';
        requestDispatcher(clientSocket, buffer, bytesReceived);
    }

    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        std::cout << "Client disconnected. Error: " << WSAGetLastError() << std::endl;
    }
}

void requestDispatcher(int receivedSocket, char buffer[1024], int bytesReceived){
    std::string received_str = std::string(buffer, bytesReceived);
    std::cout << received_str << std::endl;
    try {
        json json_received = json::parse(received_str);
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        getpeername(receivedSocket, (sockaddr *) &clientAddr, &clientAddrSize);
        char clientip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientip, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddr.sin_port);

        if (json_received["type"] == "HeartBeat") {
            std::string ip = clientip;
            int port = json_received["port"];
            serverHandler(ip, port);
        }
        if (json_received["type"] == "RegisterName") {
            std::string ip = clientip;
            int port = json_received["port"];
            std::string methodName = json_received["method"];
            updateMethod(ip, port, methodName);
        }
        if (json_received["type"] == "ServiceDiscovery") {
            std::string method = json_received["method"];
            serviceDiscover(receivedSocket, method);
        }
    } catch (std::exception &e) {
        std::cout << "Parse error: " << e.what() << std::endl;
        std::string error_response = "{\"result\": \"Invalid JSON format\"}\n";
        send(receivedSocket, error_response.c_str(), int(error_response.length()), 0);
    }
}

void acceptConn() {
    while (true) {
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(registerSocket, (sockaddr *) &clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept socket failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach(); // 将线程分离，独立运行
    }
}

void startConn(CHAR* ip, int port) {
    // 初始化WSA
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cout << "WSAStartup failed: " << result << std::endl;
        return;
    }

    // 创建套接字
    registerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (registerSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    // 设置服务器地址
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);

    // 绑定套接字
    if (bind(registerSocket, (sockaddr *) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(registerSocket);
        WSACleanup();
        return;
    }

    // 监听连接
    if (listen(registerSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(registerSocket);
        WSACleanup();
        return;
    }

    std::cout << "Register is listening on port " << port << std::endl;

    // 创建接受连接的线程
    std::thread acceptThread(acceptConn);
    acceptThread.join(); // 等待接受连接线程完成（实际永远不会完成）
}
