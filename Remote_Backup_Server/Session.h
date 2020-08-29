//
// Created by Daniele Leto on 18/08/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <filesystem>
#include <fstream>

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



    void call_read();
    void call_write(std::size_t length);

public:
    explicit Session(tcp::socket socket);
    void start();
    void doRead();
    void processRead(size_t t_bytesTransferred);
    void readData(std::istream &stream);
    int createFile();
    void doReadFileContent(size_t t_bytesTransferred);
};
