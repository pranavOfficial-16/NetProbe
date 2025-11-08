#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <unordered_map>

#include "port_status.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#include <psapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")

void checkPortStatus(const std::vector<int>& ports, bool killProcess) {
    std::cout << "Port\tStatus\tPID\tProcess\n";

    std::unordered_map<int, DWORD> usedPorts; // port → PID mapping

    for (int port : ports) {
        bool used = false;
        DWORD pid = 0;
        std::string processName = "-";

        PMIB_TCPTABLE_OWNER_PID tcpTable = (MIB_TCPTABLE_OWNER_PID*)malloc(100000);
        DWORD size = 100000;
        if (GetExtendedTcpTable(tcpTable, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
            for (DWORD i = 0; i < tcpTable->dwNumEntries; i++) {
                if (ntohs((u_short)tcpTable->table[i].dwLocalPort) == port) {
                    used = true;
                    pid = tcpTable->table[i].dwOwningPid;

                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                    if (hProcess) {
                        char name[MAX_PATH];
                        if (GetModuleBaseNameA(hProcess, nullptr, name, MAX_PATH))
                            processName = name;
                        CloseHandle(hProcess);
                    }
                    break;
                }
            }
        }
        free(tcpTable);

        if (used) {
            usedPorts[port] = pid;
            std::cout << port << "\tUsed\t" << pid << "\t" << processName << "\n";
        }
        else {
            std::cout << port << "\tIdle\t-\t-\n";
        }
    }

    if (!usedPorts.empty()) {
        std::cout << "\nDo you want to kill any process? (y/n): ";
        char choice;
        std::cin >> choice;

        if (choice == 'y' || choice == 'Y') {
            std::cin.ignore(); // clear newline from input buffer
            std::cout << "Enter the port numbers to kill (comma-separated, e.g., 8080,80): ";
            std::string portsInput;
            std::getline(std::cin, portsInput);

            std::stringstream ss(portsInput);
            std::string portStr;
            while (std::getline(ss, portStr, ',')) {
                int selectedPort = std::stoi(portStr);
                auto it = usedPorts.find(selectedPort);
                if (it != usedPorts.end()) {
                    DWORD pid = it->second;
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                    if (hProcess) {
                        TerminateProcess(hProcess, 0);
                        std::cout << "\nKilled PID " << pid << " (Port " << selectedPort << ") -> Success\n";
                        CloseHandle(hProcess);
                    }
                    else {
                        std::cout << "\nFailed to kill PID " << pid << " (Port " << selectedPort << ") ❌\n";
                    }
                }
                else {
                    std::cout << "\nNo process found using port " << selectedPort << "\n";
                }
            }
        }
        else {
            std::cout << "No processes were killed.\n";
        }
    }
    else {
        std::cout << "\nNo active ports found.\n";
    }
}

#else // ---------- Linux / macOS ----------
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <arpa/inet.h>
#include <csignal>

void checkPortStatus(const std::vector<int>& ports, bool killProcess) {
    std::cout << "Port\tStatus\tPID\tProcess\n";

    std::unordered_map<int, int> usedPorts; // port → PID mapping

    for (int port : ports) {
        bool used = false;
        int pid = -1;
        std::string procName = "-";

        std::ifstream file("/proc/net/tcp");
        std::string line;
        std::getline(file, line); // skip header

        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string localAddr, remAddr, st;
            int localPort = -1;

            iss >> std::ws;
            iss >> localAddr >> remAddr >> st;

            // 🔹 Safe parsing (fixes stoi invalid_argument)
            size_t pos = localAddr.find(':');
            if (pos == std::string::npos || pos + 1 >= localAddr.size())
                continue;

            std::string portHex = localAddr.substr(pos + 1);
            try {
                localPort = std::stoi(portHex, nullptr, 16);
            }
            catch (...) {
                continue; // skip malformed lines
            }

            if (localPort == port) {
                used = true;
                break;
            }
        }

        if (used) {
            std::string cmd = "lsof -i :" + std::to_string(port) + " -t 2>/dev/null";
            FILE* pipe = popen(cmd.c_str(), "r");
            if (pipe) {
                char buffer[64];
                if (fgets(buffer, sizeof(buffer), pipe))
                    pid = atoi(buffer);
                pclose(pipe);
            }

            if (pid > 0) {
                usedPorts[port] = pid;

                std::string path = "/proc/" + std::to_string(pid) + "/comm";
                std::ifstream pname(path);
                if (pname.is_open()) {
                    std::getline(pname, procName);
                    pname.close();
                }

                std::cout << port << "\tUsed\t" << pid << "\t" << procName << "\n";
            }
        }
        else {
            std::cout << port << "\tIdle\t-\t-\n";
        }
    }

    // 🔹 Kill prompt section (multi-port supported)
    if (!usedPorts.empty()) {
        std::cout << "\nDo you want to kill any process? (y/n): ";
        char choice;
        std::cin >> choice;

        if (choice == 'y' || choice == 'Y') {
            std::cin.ignore();
            std::cout << "Enter the port numbers to kill (comma-separated, e.g., 8080,80): ";
            std::string portsInput;
            std::getline(std::cin, portsInput);

            std::stringstream ss(portsInput);
            std::string portStr;
            while (std::getline(ss, portStr, ',')) {
                int selectedPort = -1;
                try {
                    selectedPort = std::stoi(portStr);
                }
                catch (...) {
                    std::cout << "Invalid port: " << portStr << "\n";
                    continue;
                }

                auto it = usedPorts.find(selectedPort);
                if (it != usedPorts.end()) {
                    int pid = it->second;
                    if (kill(pid, SIGTERM) == 0)
                        std::cout << "→ Killed PID " << pid << " (Port " << selectedPort << ") ✅\n";
                    else
                        std::cout << "→ Failed to kill PID " << pid << " (Port " << selectedPort << ") ❌\n";
                }
                else {
                    std::cout << "No process found using port " << selectedPort << "\n";
                }
            }
        }
        else {
            std::cout << "No processes were killed.\n";
        }
    }
    else {
        std::cout << "\nNo active ports found.\n";
    }
}
#endif
