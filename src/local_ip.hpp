#pragma once
#include <string>

std::string getLocalIP();
bool sameNetwork(const std::string& local, const std::string& remote);
