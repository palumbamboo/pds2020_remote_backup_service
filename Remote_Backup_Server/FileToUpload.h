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
    size_t fileSize;
public:
    FileToUpload()=default;
    FileToUpload(std::filesystem::path _path, size_t _filesize) : path(std::move(_path)), fileSize(_filesize) {}
    ~FileToUpload()=default;
    std::filesystem::path getPath() { return path; }
    std::string getPathName() { return path.filename().string(); }
    std::string fileHash();
    size_t getFileSize() { return fileSize; }
    void setPath(const std::filesystem::path &path);
    void setFileSize(size_t fileSize);
};


#endif //REMOTE_BACKUP_CLIENT_FILETOUPLOAD_H
