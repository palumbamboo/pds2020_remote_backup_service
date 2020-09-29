//
// Created by daniele on 28/09/20.
//

#ifndef REMOTE_BACKUP_SERVER_USERMAP_H
#define REMOTE_BACKUP_SERVER_USERMAP_H

#include <map>
#include <vector>
#include <mutex>
#define PASS_PATH       "password_server.json"

extern std::map<std::string, std::vector<std::string>> userMap;
extern std::map<std::string, std::vector<std::string>> userFilesMap;

extern std::mutex mutex;

#endif //REMOTE_BACKUP_SERVER_USERMAP_H
