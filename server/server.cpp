#include <iostream>
#include <cstring>
#include <string>
#include "RPCServer.hpp"
using namespace std;
string greeting(string name){
    string message = "Hello" + name + "!" + " This is server. Congratulations on taking your first step in RPC QwQ";
    return message;
};

int add(int a, int b){
    return a + b;
}

std::string adeadman(){
    return "事已至此，先睡觉吧 晚安喵";
}

std::string reverseString(std::string str) {
    std::string reversedStr = str;
    std::reverse(reversedStr.begin(), reversedStr.end());
    return reversedStr;
}

int factorial(int n) {
    if (n == 0 || n == 1)
        return 1;
    else
        return n * factorial(n - 1);
}

bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }
    return true;
}


int gcdCal(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

bool isPalindrome(std::string str) {
    std::string reversedStr = str;
    std::reverse(reversedStr.begin(), reversedStr.end());
    return str == reversedStr;
}


string sleep(){
    for (int i = 0; i < 5; ++i) {
        std::cout << "Running for " << i + 1 << " seconds..." << std::endl;
        Sleep(1000);
    }
    string message = "sleep for Q5Q seconds";
    return message;
}
int main(int argc, char *argv[]) {
    string listenIP = "127.0.0.1";
    int port = -1;
    string remoteRegiIP = "127.0.0.1"; // 默认远程注册中心 IP
    int remoteRegiPort = 9999; // 默认远程注册中心端口号
    bool shouldConnectRemoteRegi = false; // 标志是否要执行 connectRemoteRegi

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0) {
            // 输出帮助信息
            cout << "Usage: " << argv[0] << " [-h] [-l <listen_ip>] -p <port> [-ri <remote_regi_ip>] [-rp <remote_regi_port>]" << endl;
            cout << "Options:" << endl;
            cout << "  -h                 Display this help message" << endl;
            cout << "  -l <listen_ip>     IP address to listen on (default: 0.0.0.0)" << endl;
            cout << "  -p <port>          Port number to listen on (required)" << endl;
            cout << "  -ri <remote_regi_ip>  IP address of remote registry center (default: 127.0.0.1)" << endl;
            cout << "  -rp <remote_regi_port>  Port number of remote registry center (default: 9999)" << endl;
            return 0;
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            // 设置监听 IP 地址
            listenIP = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            // 设置端口号
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ri") == 0 && i + 1 < argc) {
            // 设置远程注册中心 IP 地址
            remoteRegiIP = argv[++i];
            shouldConnectRemoteRegi = true; // 指示需要执行 connectRemoteRegi
        } else if (strcmp(argv[i], "-rp") == 0 && i + 1 < argc) {
            // 设置远程注册中心端口号
            remoteRegiPort = atoi(argv[++i]);
            shouldConnectRemoteRegi = true; // 指示需要执行 connectRemoteRegi
        }
    }

    // 检查端口号是否为空
    if (port == -1) {
        cerr << "Error: Port number is required. Use -h option for help." << endl;
        return 1;
    }

    // 创建 RPC 服务器并注册函数
    RPCServer server(const_cast<CHAR*>(listenIP.c_str()), port); // 将 string 转为 const char*
    std::cout << "监听端口：" << port << std::endl;

    // 如果需要，连接到远程注册中心
    if (shouldConnectRemoteRegi) {
        server.connectRemoteRegi(const_cast<CHAR*>(remoteRegiIP.c_str()), remoteRegiPort);
    }

    server.regi("SayHello", greeting);
    server.regi("add", add);
    server.regi("sleep", sleep);
    server.regi("factorial", factorial);
    server.regi("isPrime", isPrime);
    server.regi("isPalindrome", isPalindrome);
    server.regi("gcd", gcdCal);
    server.regi("reverseString", reverseString);
    server.regi("XieBuWanLe", adeadman);
    server.start();
}