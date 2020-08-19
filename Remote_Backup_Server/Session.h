//
// Created by Daniele Leto on 18/08/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <filesystem>
#include <array>
#include <fstream>
#include <string>
#include <memory>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
private:
    tcp::socket socket;
    enum {MaxLength = 40960};
    std::array<char, MaxLength> buffer;
    size_t m_fileSize;
    std::string m_fileName;
    boost::asio::streambuf m_requestBuf_;
    std::ofstream m_outputFile;

    void call_read();
    void callWrite(std::size_t length);
    void processRead(size_t t_bytesTransferred);
    void createFile();
    void readData(std::istream &stream);
    void doReadFileContent(size_t t_bytesTransferred);
    void handleError(std::string const& t_functionName, boost::system::error_code const& t_ec);

public:
    explicit Session(tcp::socket socket);

    void start();
};
