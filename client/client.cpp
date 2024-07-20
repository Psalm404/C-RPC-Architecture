#include <iostream>
#include <cstring>
#include <RPCClient.hpp>
#include <cstdio>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

void usage() {
    std::cout << "Usage: client.exe -h -i <registry_center_ip> -p <registry_center_port> [--connectremoteregi]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h               Display this help message" << std::endl;
    std::cout << "  -i <remote_regi_ip>   Specify the IP address" << std::endl;
    std::cout << "  -p <remote_regi_port> Specify the port number" << std::endl;
    std::cout << "  --ConnectRemoteRegi   Specify if the client should connect to remote registry" << std::endl;
}

int main(int argc, char* argv[]) {
    string serverIP;
    int serverPort = -1;
    bool connectRemoteRegi = false;

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            serverIP = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            serverPort = stoi(argv[++i]);
        } else if (strcmp(argv[i], "--ConnectRemoteRegi") == 0) {
            connectRemoteRegi = true;
        } else {
            cerr << "Invalid option or missing argument: " << argv[i] << endl;
            usage();
            return 1;
        }
    }

    // 检查是否提供了必需的选项
    if (serverIP.empty() || serverPort == -1) {
        cerr << "Missing required options: -i and -p" << endl;
        usage();
        return 1;
    }

    RPCClient client(const_cast<CHAR*>(serverIP.c_str()), serverPort, connectRemoteRegi);
    auto result = client.call<string>("SayHello", "Psalm");
    // 调用远程函数并赋值给变量
    int result1 = client.call<int>("add", 9, 3);
    std::cout << "Result from add(9, 3): " << result1 << std::endl;
    auto result2 = client.call<string>("XieBuWanLe");
    std::cout << "Result from XieBuWanLe: " << result2 << std::endl;
    auto result3 = client.call<string>("reverseString", "abcdefg");
    std::cout << "Result from reverseString(\"abcdefg\"): " << result3 << std::endl;
    auto result4 = client.call<int>("factorial", 5);
    std::cout << "Result from factorial(5): " << result4 << std::endl;
    auto result5 = client.call<bool>("isPrime", 13);
    std::cout << "Result from isPrime(13): " << result5 << std::endl;
    auto result6 = client.call<int>("gcd", 992, 524);
    std::cout << "Result from gcd(992, 524): " << result6 << std::endl;
    auto result7 = client.call<bool>("isPalindrome", "12321");
    std::cout << "Result from isPalindrome(\"12321\"): " << result7 << std::endl;
    auto result8 = client.call<string>("sleep");
    std::cout << "Result from sleep: " << result8 << std::endl;

}
