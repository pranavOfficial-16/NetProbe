#include <string>
#include <cstdlib>   

#include "reachability.hpp"

bool checkReachability(const std::string& ip) {
#ifdef _WIN32
    // Windows: -n 1 means one ping, -w 1000 sets timeout to 1 second
    std::string command = "ping -n 1 -w 1000 " + ip + " >nul 2>&1";
#else
    // Linux/macOS: -c 1 means one ping, -W 1 sets timeout to 1 second
    std::string command = "ping -c 1 -W 1 " + ip + " >/dev/null 2>&1";
#endif
    int result = std::system(command.c_str());
    return result == 0;  // 0 = success (reachable)
}
