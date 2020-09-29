//
// Created by daniele on 28/09/20.
//

#include "UserMap.h"

std::map<std::string, std::vector<std::string>> userMap{};
std::map<std::string, std::vector<std::string>> userFilesMap{};

std::mutex mutex;
