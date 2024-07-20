#include <unordered_set>
#include "json.hpp"
#include "LocalRegister.hpp"
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <ThreadPool.hpp>
#include <thread> // 添加 std::thread 头文件
#pragma comment(lib, "Ws2_32.lib")


std::mutex IOmutex;
SOCKET serverSocket{};
typedef struct fdinfo{
    int fd;
    int *max_fd;
    fd_set *rdset;
    std::unordered_set<int> *workSet;
} FDInfo;

ThreadPool threadPool(10); // 可以指定线程池中的最大线程数

void startConn(CHAR* ip, int port);
json handleFunctionCall(const json& request);
void handleClient(SOCKET clientSocket, char buffer[1024], int bytesReceived);
void IOSelect();
void acceptConn(FDInfo* info);
void communication(FDInfo* info);

void acceptConn(FDInfo* info){
    auto start = std::chrono::high_resolution_clock::now();
    std::unique_lock<std::mutex> lock(IOmutex);
    //std::cout << "accept:threadID:" << std::this_thread::get_id() << std::endl;
    sockaddr_in clientAddr{};
    int clientAddrSize = sizeof(clientAddr);
    SOCKET clientSocket = accept(info->fd, (sockaddr *) &clientAddr, &clientAddrSize);
    lock.unlock();
    FD_SET(clientSocket, info->rdset);
    *info->max_fd = *info->max_fd > int(clientSocket) ? *info->max_fd : int(clientSocket);
    (*info->workSet).erase(info->fd);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    // std::cout << "Time taken by accept: " << duration.count() << " microseconds" << std::endl;
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Accept socket 失败 " << WSAGetLastError() <<  std::endl;
        delete info;
        return;
    }
    getpeername(clientSocket, (sockaddr *) &clientAddr, &clientAddrSize);
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddr.sin_port);
    // cout << "Client connected: " << clientIP << ":" << clientPort << endl;
    delete info;
}

void communication(FDInfo* info){
    char buffer[1024];
    int bytesReceived = recv(info->fd, buffer, sizeof(buffer), 0);

    sockaddr_in clientAddr{};
    int clientAddrSize = sizeof(clientAddr);
    getpeername(info->fd, (sockaddr *) &clientAddr, &clientAddrSize);
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddr.sin_port);

    if (bytesReceived <= 0) {
        closesocket(info->fd);
        std::unique_lock<std::mutex> lock(IOmutex);
        FD_CLR(info->fd, info->rdset);
        lock.unlock();
        if (info->fd == *info->max_fd){
            for (int ii = *info->max_fd; ii > 0; ii--){
                if (FD_ISSET(ii, info->rdset)){
                    std::unique_lock<std::mutex> lock(IOmutex);
                    *info->max_fd = ii;
                    lock.unlock();
                    break;
                }
            }
        }
        if (bytesReceived == 0)
            std::cout << "Client " << clientIP << ":" << clientPort << " 断开连接 " << std::endl;
        (*info->workSet).erase(info->fd);
        delete info;
        return;
    } else {
        // std::cout << "Received Message from Client " << clientIP << ":" << clientPort << ": ";
        buffer[bytesReceived] = '\0';
        handleClient(info->fd, buffer, bytesReceived);
    }
    (*info->workSet).erase(info->fd);
    delete info;
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
        std::cout<<"服务端正在监听所有接口"<<std::endl;
    } else {
        inet_pton(AF_INET, ip, &serverAddr.sin_addr); // 使用指定的监听 IP
    }

    // 绑定套接字
    auto bind_result = bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (bind_result < 0){
        std::cout<<"bind failed!"<<std::endl;
    }
    // 监听连接
    listen(serverSocket, SOMAXCONN);

    //std::cout << "服务端正在监听端口" << port << std::endl;

    IOSelect();
}

void IOSelect() {
    std::unordered_set<int> workSet; // 维护一个集合表示正在处理的文件描述符序列
    fd_set read_set;
    int max_fd;
    FD_ZERO(&read_set);
    FD_SET(serverSocket, &read_set);
    max_fd = int(serverSocket);
    while(1){
        std::unique_lock<std::mutex> lock(IOmutex);
        fd_set temp_set = read_set;
        lock.unlock();

        int ret = select(max_fd + 1, &temp_set, NULL, NULL, NULL);
        if (ret <= 0){
            std::cout << "select()出现错误" << std::endl;
            std::cout << WSAGetLastError() << std::endl;
            continue;
        }
        for (int event_fd = 0; event_fd <= max_fd; event_fd++){
            if (FD_ISSET(event_fd, &temp_set) <= 0) continue;
            if (workSet.count(event_fd) == 0){ // 如果该文件描述符不在工作集合里，就添加进等待列表
                workSet.insert(event_fd);
                if (event_fd == serverSocket){
                    auto* info = new FDInfo{event_fd, &max_fd, &read_set, &workSet};
                    threadPool.enqueue(acceptConn, info);
                } else {
                    auto* info = new FDInfo{event_fd, &max_fd, &read_set, &workSet};
                    threadPool.enqueue(communication, info);
                }
            }
        }
        Sleep(5);
    }
}

void handleClient(SOCKET clientSocket, char buffer[1024], int bytesReceived) {
    // 处理客户端连接
    std::string request_str = std::string(buffer, bytesReceived);
    std::cout<<request_str<<std::endl;
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

json handleFunctionCall(const json &request) {
    std::string method = request["method"];
    const json& params = request["params"];
    if (LocalRegister::funcMap.find(method) !=  LocalRegister::funcMap.end()) {
        std::cout<<"find! "<<method<<std::endl;
        return { {"result",  LocalRegister::funcMap[method](params)} };
    }
    std::cout<<"not found "<<method<<" QAQ "<<std::endl;
    return { {"result", "Method not found"} };
}

