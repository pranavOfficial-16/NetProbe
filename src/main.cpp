#include <iostream>
#include <vector>
#include <string>

#include "reachability.hpp"
#include "local_ip.hpp"
#include "port_status.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: netprobe <remote_ip> [--ports 22,80,8080]" << std::endl;
        return 1;
    }

    std::string remote_ip = argv[1];
    std::vector<int> ports;

    // Parse ports from command line if provided
    if (argc >= 4 && std::string(argv[2]) == "--ports") {
        std::string port_list = argv[3];
        size_t pos = 0;
        while ((pos = port_list.find(',')) != std::string::npos) {
            ports.push_back(std::stoi(port_list.substr(0, pos)));
            port_list.erase(0, pos + 1);
        }
        ports.push_back(std::stoi(port_list)); // last one
    }
    else
        ports = { 80 }; // default

    std::cout << "\nRemote IP: " << remote_ip << " -> " << (checkReachability(remote_ip) ? "Reachable" : "Unreachable") << std::endl;
    std::string local_ip = getLocalIP();
    std::cout << "Local IP: " << local_ip << " -> " << (sameNetwork(local_ip, remote_ip) ? "Same network" : "Different network") << std::endl;

    std::cout << "\nPort Status:\n";
    checkPortStatus(ports);
    std::cout << "\n";

    return 0;
}
