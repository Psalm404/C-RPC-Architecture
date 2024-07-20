#include "json.hpp"
#include <ws2tcpip.h>
#include <winsock2.h>
#include <cstring>
#include "ServerHandler.hpp"
#pragma comment(lib, "Ws2_32.lib")
using json = nlohmann::json;
SOCKET registerSocket;
char clientip[INET_ADDRSTRLEN];
int clientPort;
// 对接收到的请求分类
void requestDispatcher(int receivedSocket, char buffer[1024], int bytesReceived){
    std::string received_str = std::string(buffer, bytesReceived);
    std::cout<<received_str<<std::endl;
    try {
        json json_received = json::parse(received_str);
        if (json_received["type"] == "HeartBeat"){
            std::string ip = clientip;
            int port = json_received["port"];
            serverHandler(ip, port);
        }
        if (json_received["type"] == "RegisterName"){
            std::string ip = clientip;
            int port = json_received["port"];
            std::string methodName = json_received["method"];
            updateMethod(ip, port, methodName);
        }
        if (json_received["type"] == "ServiceDiscovery"){
            std::string method = json_received["method"];
            serviceDiscover(receivedSocket, method);
        }
    } catch (std::exception &e) {
        std::cout << "Parse error: " << e.what() << std::endl;
        std::string error_response = "{\"result\": \"Invalid JSON format\"}\n";
        send(receivedSocket, error_response.c_str(), int(error_response.length()), 0);
    }
}

void IOSelect() {
    fd_set read_set;
    int max_fd;
    FD_ZERO(&read_set);
    FD_SET(registerSocket, &read_set);
    max_fd = int(registerSocket);
    while(1){
        fd_set temp_set = read_set;
        int ret = select(max_fd + 1, &temp_set, NULL, NULL, NULL);
        //std::cout << ret << " socket is ready" << std::endl;
        if (ret <= 0){
            std::cout << "执行select()方法时出现错误" << std::endl;
            std::cout << WSAGetLastError() << std::endl;
            continue;
        }
        for (int event_fd = 0; event_fd <= max_fd; event_fd++){
            if (FD_ISSET(event_fd, &temp_set) <= 0) continue;
           // std::cout << "event_fd: " << event_fd <<  std::endl;
                if (event_fd == registerSocket){
                    //std::cout << "accept:threadID:" << pthread_self() << std::endl;
                    sockaddr_in clientAddr{};
                    int clientAddrSize = sizeof(clientAddr);
                    SOCKET clientSocket = accept(event_fd, (sockaddr *) &clientAddr, &clientAddrSize);
                    FD_SET(clientSocket, &read_set);
                    max_fd = max_fd > int(clientSocket) ? max_fd : int(clientSocket);
                    if (clientSocket == INVALID_SOCKET) {
                        std::cout << "套接字出现错误 " << WSAGetLastError() << std::endl;
                    }

                    getpeername(clientSocket, (sockaddr *) &clientAddr, &clientAddrSize);
                    inet_ntop(AF_INET, &clientAddr.sin_addr, clientip, INET_ADDRSTRLEN);
                    clientPort = ntohs(clientAddr.sin_port);
                    //std::cout << "Client connected: " << clientip << ":" << clientPort << std::endl;
                } else {
                    char buffer[1024];
                    int bytesReceived = recv(event_fd, buffer, sizeof(buffer), 0);

                    sockaddr_in clientAddr{};
                    int clientAddrSize = sizeof(clientAddr);

                    getpeername(event_fd, (sockaddr *) &clientAddr, &clientAddrSize);
                    inet_ntop(AF_INET, &clientAddr.sin_addr, clientip, INET_ADDRSTRLEN);
                    clientPort = ntohs(clientAddr.sin_port);

                    if (bytesReceived <= 0) {
                        closesocket(event_fd);
                        FD_CLR(event_fd, &read_set);
                        if (event_fd == max_fd){
                            for (int ii =max_fd; ii > 0; ii--){
                                if (FD_ISSET(ii, &read_set)){
                                    max_fd = ii;
                                    break;
                                }
                            }
                        }
                        if (bytesReceived == 0){
                            std::cout << clientip << ":" << clientPort << " 断开连接 " << std::endl;
                        }

                    } else {
                        std::cout << "Received Message from " << clientip << ":" << clientPort << ": ";
                        buffer[bytesReceived] = '\0';
                        requestDispatcher(event_fd, buffer, bytesReceived);
                    }
            }
        }
    }
}

void startConn(CHAR* ip, int port) {
    //初始化WSA
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cout << "WSAStartup failed: " << result << std::endl;
        return;
    }
    //创建套接字
    registerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (registerSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    //设置服务器地址
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);

    // 绑定套接字
    bind(registerSocket, (sockaddr *) &serverAddr, sizeof(serverAddr));

    // 监听连接
    listen(registerSocket, SOMAXCONN);

    std::cout << "register is listening on port " << port << std::endl;
    std::thread timeoutThread(checkTimeouts);
    IOSelect();
}
