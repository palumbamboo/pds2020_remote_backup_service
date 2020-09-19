//
// Created by daniele on 19/09/20.
//

#ifndef REMOTE_BACKUP_CLIENT_FILETOUPLOAD_H
#define REMOTE_BACKUP_CLIENT_FILETOUPLOAD_H

#include <filesystem>
#include <fstream>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>
#include <utility>
#include <iostream>

class FileToUpload {
private:
    std::filesystem::path path;
public:
    FileToUpload(std::filesystem::path _path) : path(std::move(_path)) {}
    ~FileToUpload()=default;
    std::filesystem::path getPath() { return path; }
    std::string getPathName() { return path.filename().string(); }
    std::string fileHash();
};


#endif //REMOTE_BACKUP_CLIENT_FILETOUPLOAD_H
