//
// Created by daniele on 28/09/20.
//

#include "UserMap.h"

std::map<std::string, std::vector<std::string>> userMap{};
std::map<std::string, std::vector<std::string>> userFilesMap{};

std::mutex mutexUserMap{};
std::mutex mutexUserFilesMap{};

void insertUserLogin(const std::string &userName, std::vector<std::string> &userData) {
    std::lock_guard<std::mutex> lockGuard(mutexUserMap);
    userMap.insert(std::pair<std::string, std::vector<std::string>>(userName, userData));
}

bool findUserInUserLoginMap(const std::string &userName) {
    std::lock_guard<std::mutex> lockGuard(mutexUserMap);
    auto it = userMap.find(userName);
    return it != userMap.end();
}

std::vector<std::string> getUserInUserLoginMap(const std::string &userName) {
    std::lock_guard<std::mutex> lockGuard(mutexUserMap);
    return userMap[userName];
}

void saveUserLoginMapToFile() {
    std::lock_guard<std::mutex> lockGuard(mutexUserMap);
    nlohmann::json jsonMap(userMap);
    std::ofstream o(PASS_PATH);
    o << std::setw(4) << jsonMap << std::endl;
}

void loadUserLoginMap() {
    std::lock_guard<std::mutex> lockGuard(mutexUserMap);
    std::ifstream passFile(PASS_PATH);
    nlohmann::json jsonMap;
    passFile >> jsonMap;
    userMap = jsonMap.get<std::map<std::string, std::vector<std::string>>>();
    passFile.close();
}


void insertUserFilesMap(const std::string &clientID, std::vector<std::string> &filesVector) {
    std::lock_guard<std::mutex> lockGuard(mutexUserFilesMap);
    userFilesMap.insert(std::pair<std::string, std::vector<std::string>>(clientID, filesVector));
}

bool findUserInUserFilesMap(const std::string &clientID) {
    std::lock_guard<std::mutex> lockGuard(mutexUserFilesMap);
    auto it = userFilesMap.find(clientID);
    return it != userFilesMap.end();
}

void addFileToUserFilesMap(const std::string &clientID, const std::string &fileName) {
    std::lock_guard<std::mutex> lockGuard(mutexUserFilesMap);
    userFilesMap[clientID].push_back(fileName);
}

std::vector<std::string> getFilesInUserFilesMap(const std::string &clientID) {
    std::lock_guard<std::mutex> lockGuard(mutexUserFilesMap);
    return userFilesMap[clientID];
}

void deleteFromUserFilesMap(const std::string &clientID) {
    std::lock_guard<std::mutex> lockGuard(mutexUserFilesMap);
    userFilesMap.erase(clientID);
}
