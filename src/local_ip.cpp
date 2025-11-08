#include <iostream>
#include <cstring>

#include "local_ip.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <ifaddrs.h>
#include <arpa/inet.h>
#endif

std::string getLocalIP() {
#ifdef _WIN32
    char buffer[256];
    gethostname(buffer, sizeof(buffer));
    struct hostent* host = gethostbyname(buffer);
    if (host && host->h_addr_list[0]) {
        return inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
    }
    return "Unknown";
#else
    struct ifaddrs* ifaddr, * ifa;
    char addr[INET_ADDRSTRLEN];

    if (getifaddrs(&ifaddr) == -1)
        return "Unknown";

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            void* in_addr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, in_addr, addr, sizeof(addr));
            std::string ip = addr;
            if (ip != "127.0.0.1") { // skip localhost
                freeifaddrs(ifaddr);
                return ip;
            }
        }
    }
    freeifaddrs(ifaddr);
    return "Unknown";
#endif
}

bool sameNetwork(const std::string& local, const std::string& remote) {
    // Handle loopback as same network for simplicity
    if (remote == "127.0.0.1" || remote.find("127.") == 0)
        return true;

    return local.substr(0, local.rfind('.')) == remote.substr(0, remote.rfind('.'));
}

