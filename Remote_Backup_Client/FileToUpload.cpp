//
// Created by daniele on 19/09/20.
//

#include "FileToUpload.h"

std::string FileToUpload::fileHash() {
    std::ifstream file;
    file.open(path, std::ios::in);
    if (file.fail())
        throw std::fstream::failure("Failed while opening file " + path.string() + "\n");

    std::string s;
    std::string hashResult;
    while(std::getline(file, s)) {
        boost::uuids::detail::md5 hash;
        boost::uuids::detail::md5::digest_type digest;

        hash.process_bytes(s.data(), s.size());
        hash.get_digest(digest);

        std::string result;

        const auto charDigest = reinterpret_cast<const char *>(&digest);
        boost::algorithm::hex(charDigest, charDigest + sizeof(boost::uuids::detail::md5::digest_type),
                              std::back_inserter(result));
        hashResult = result;
    }
    hash = hashResult;
    return hash;
}

void FileToUpload::setPath(const std::filesystem::path &_path) {
    path = _path;
}

void FileToUpload::setFileSize(size_t _fileSize) {
    fileSize = _fileSize;
}

std::filesystem::path FileToUpload::getPathToUpload() {
    std::string pathToTrim = path;
    size_t pos = pathToTrim.find(folderToWatch);
    if (pos != std::string::npos)
    {
        // If found then erase it from string
        pathToTrim.erase(pos, folderToWatch.length());
    }
    return std::filesystem::path(pathToTrim);
}
