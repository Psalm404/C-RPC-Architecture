#include <cstdio>
#include <iostream>
#include <string>
#include "DispatcherServlet.hpp"
#include <cstring>
#include <cstdlib>

void printHelp() {
    std::cout << "Usage: ./registry_center -l <ip_address> -p <port> [-h]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -l <ip_address>\tSpecify IP address to bind (default: 127.0.0.1)." << std::endl;
    std::cout << "  -p <port>\t\tSpecify port number to bind (required)." << std::endl;
    std::cout << "  -h\t\t\tPrint this help message." << std::endl;
}

int main(int argc, char* argv[]) {
    char* bindAddress = "127.0.0.1"; // Default bind address
    int bindPort = 0; // Default port
    bool printHelpFlag = false;

    // Parse command line arguments
    if (argc < 2) {
        std::cerr << "Error: Insufficient arguments." << std::endl;
        printHelp();
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0) {
            printHelpFlag = true;
        } else if (std::strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            bindAddress = argv[i + 1];
            ++i;
        } else if (std::strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            bindPort = std::atoi(argv[i + 1]);
            ++i;
        } else {
            std::cerr << "Error: Unknown option or missing argument." << std::endl;
            printHelp();
            return 1;
        }
    }

    if (printHelpFlag) {
        printHelp();
        return 0;
    }

    if (bindPort == 0) {
        std::cerr << "Error: Missing required -p option with port number." << std::endl;
        printHelp();
        return 1;
    }

    startConn(bindAddress, bindPort);

    return 0;
}