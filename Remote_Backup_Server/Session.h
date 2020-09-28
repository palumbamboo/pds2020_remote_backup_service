//
// Created by Daniele Leto on 18/08/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <filesystem>
#include <fstream>
#include "Message.h"
#include "FileToUpload.h"
#include "UserMap.h"
#include <nlohmann/json.hpp>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
private:
    tcp::socket socket;
    enum {MaxLength = 40000};
    boost::asio::streambuf m_requestBuf_;
    std::array<char, MaxLength> m_buf;
    std::filesystem::path m_fileName;
    std::ofstream m_outputFile;
    size_t m_fileSize;
    std::string m_task;
    std::string m_clientId;
    Message m_message;
    FileToUpload m_fileToUpload;
    std::string m_fileHash;
    bool m_response;
    std::string m_username;
    std::string m_hashedPassword;

    template<class Buffer>
    void writeBuffer(Buffer& t_buffer);

public:
    explicit Session(tcp::socket socket);
    void start();
    void doRead();
    void processRead(size_t t_bytesTransferred);
    void readData(std::istream &stream);
    int createFile();
    void doReadFileContent(size_t t_bytesTransferred);
    void doWriteResponse();
};

template<class Buffer>
void Session::writeBuffer(Buffer& t_buffer)
{
    boost::asio::async_write(socket,
                             t_buffer,
                             [this](boost::system::error_code ec, size_t /*length*/)
                             {
                                 std::cout << "Inside Write Buffer..................." << std::endl;
                             });
}