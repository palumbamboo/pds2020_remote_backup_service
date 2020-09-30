//
// Created by daniele on 28/09/20.
//

#ifndef REMOTE_BACKUP_SERVER_USERMAP_H
#define REMOTE_BACKUP_SERVER_USERMAP_H

#include <map>
#include <vector>
#include <mutex>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#define PASS_PATH "password_server.json"

extern std::map<std::string, std::vector<std::string>> userMap;
extern std::map<std::string, std::vector<std::string>> userFilesMap;

extern std::mutex mutexUserMap;
extern std::mutex mutexUserFilesMap;

void insertUserLogin(const std::string &userName, std::vector<std::string> &userData);
bool findUserInUserLoginMap(const std::string &userName);
std::vector<std::string> getUserInUserLoginMap(const std::string &userName);
void saveUserLoginMapToFile();

void insertUserFilesMap(const std::string &clientID, std::vector<std::string> &filesVector);
bool findUserInUserFilesMap(const std::string &clientID);
void addFileToUserFilesMap(const std::string &clientID, const std::string &fileName);
std::vector<std::string> getFilesInUserFilesMap(const std::string &clientID);
void deleteFromUserFilesMap(const std::string &clientID);

#endif //REMOTE_BACKUP_SERVER_USERMAP_H
