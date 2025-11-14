#include "local_ip.hpp"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

std::string getLocalIP() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[DEBUG] WSAStartup() failed\n";
        return "Unknown";
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cerr << "[DEBUG] gethostname() failed. Error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return "Unknown";
    }

    addrinfo hints = { 0 }, * info = nullptr;
    hints.ai_family = AF_INET; // IPv4 only
    if (getaddrinfo(hostname, nullptr, &hints, &info) != 0) {
        std::cerr << "[DEBUG] getaddrinfo() failed. Error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return "Unknown";
    }

    char ip[INET_ADDRSTRLEN];
    sockaddr_in* addr = (sockaddr_in*)info->ai_addr;
    inet_ntop(AF_INET, &(addr->sin_addr), ip, sizeof(ip));


    freeaddrinfo(info);
    WSACleanup();
    return std::string(ip);
}

#else // ---------- Linux / WSL ----------
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>

std::string getLocalIP() {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        std::cerr << "[DEBUG] getifaddrs() failed\n";
        return "Unknown";
    }

    char addr[INET_ADDRSTRLEN];
    std::string fallback = "Unknown";

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            void* in_addr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, in_addr, addr, sizeof(addr));
            std::string ip = addr;
            std::cout << "[DEBUG] Interface: " << ifa->ifa_name << " | IP: " << ip << std::endl;

            if (ip != "127.0.0.1") {
                freeifaddrs(ifaddr);
                std::cout << "[DEBUG] Returning IP: " << ip << std::endl;
                return ip;
            }

            fallback = ip; // store 127.0.0.1 as last resort
        }
    }

    freeifaddrs(ifaddr);

    // WSL fallback — read Windows host IP from /etc/resolv.conf
    if (fallback == "127.0.0.1" || fallback == "Unknown") {
        std::ifstream resolv("/etc/resolv.conf");
        std::string line;
        while (std::getline(resolv, line)) {
            if (line.find("nameserver") != std::string::npos) {
                std::istringstream iss(line);
                std::string dummy, ip;
                iss >> dummy >> ip;
                std::cout << "[DEBUG] WSL detected, returning Windows host IP: " << ip << std::endl;
                return ip;
            }
        }
    }

    std::cout << "[DEBUG] Returning fallback: " << fallback << std::endl;
    return fallback;
}
#endif

bool sameNetwork(const std::string& local, const std::string& remote) {
    // Handle loopback as same network
    if (remote == "127.0.0.1" || remote.find("127.") == 0)
        return true;

    return local.substr(0, local.rfind('.')) == remote.substr(0, remote.rfind('.'));
}
