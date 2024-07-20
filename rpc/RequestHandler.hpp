#include <unordered_set>
#include "json.hpp"
#include "LocalRegister.hpp"
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#pragma comment(lib, "Ws2_32.lib")

SOCKET serverSocket{};

void startConn(CHAR* ip, int port);
json handleFunctionCall(const json& request);
void handleClient(SOCKET clientSocket);
void acceptConn();

void acceptConn(){
    while (true) {
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr *) &clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept socket 失败 " << WSAGetLastError() <<  std::endl;
            continue;
        }

        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach(); // 将线程分离，独立运行
    }
}

void handleClient(SOCKET clientSocket){
    char buffer[1024];
    int bytesReceived;

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        getpeername(clientSocket, (sockaddr *) &clientAddr, &clientAddrSize);
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddr.sin_port);

        buffer[bytesReceived] = '\0';
        std::string request_str = std::string(buffer, bytesReceived);
        std::cout << request_str << std::endl;

        try {
            json json_request = json::parse(request_str);
            auto response_str = handleFunctionCall(json_request).dump() + "\n";
            send(clientSocket, response_str.c_str(), int(std::strlen(response_str.c_str())), 0);
        } catch (json::parse_error &e) {
            std::cout << "Parse error: " << e.what() << std::endl;
            std::string error_response = "{\"result\": \"Invalid JSON format\"}\n";
            send(clientSocket, error_response.c_str(), int(error_response.length()), 0);
        }
    }

    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        std::cout << "Client 断开连接 " << WSAGetLastError() << std::endl;
    }
}

void startConn(CHAR* ip, int port) {
    // 初始化WSA
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cout << "WSAStartup 失败： " << result <<  std::endl;
        return;
    }

    // 创建套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Socket creation 失败： " << WSAGetLastError() <<  std::endl;
        WSACleanup();
        return;
    }

    // 设置服务器地址
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    // 判断是否监听所有接口
    if (strcmp(ip, "0.0.0.0") == 0) {
        serverAddr.sin_addr.s_addr = INADDR_ANY; // 监听所有网络接口
        std::cout << "服务端正在监听所有接口" << std::endl;
    } else {
        inet_pton(AF_INET, ip, &serverAddr.sin_addr); // 使用指定的监听 IP
    }

    // 绑定套接字
    auto bind_result = bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (bind_result < 0){
        std::cout << "bind failed!" << std::endl;
        return;
    }

    // 监听连接
    listen(serverSocket, SOMAXCONN);

    // 创建接受连接的线程
    std::thread acceptThread(acceptConn);
    acceptThread.join(); // 等待接受连接线程完成（实际永远不会完成）
}

json handleFunctionCall(const json &request) {
    std::string method = request["method"];
    const json& params = request["params"];
    if (LocalRegister::funcMap.find(method) != LocalRegister::funcMap.end()) {
        std::cout << "find! " << method << std::endl;
        return { {"result", LocalRegister::funcMap[method](params)} };
    }
    std::cout << "not found " << method << " QAQ " << std::endl;
    return { {"result", "Method not found"} };
}