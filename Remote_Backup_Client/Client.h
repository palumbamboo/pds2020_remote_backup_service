//
// Created by Daniele Leto on 17/08/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <fstream>

using boost::asio::ip::tcp;
enum {MaxLength = 40000};

class Client {
private:
    tcp::socket socket;
    tcp::resolver::results_type endpointIterator;

    std::array<char, MaxLength> m_buf;
    boost::asio::streambuf request;
    boost::asio::streambuf m_request;
    std::ifstream m_sourceFile;
    std::string m_path;

    template<class Buffer>
    void writeBuffer(Buffer& t_buffer);

public:

    Client(boost::asio::io_service& ioService,
           tcp::resolver::results_type endpointIterator);
    void call_connect();
    //void write_buffer(std::string buf);
    ~Client();
    void openFile(std::string const& t_path);
    //void call_write_file(const boost::system::error_code& ec);
    void doWriteFile(const boost::system::error_code& t_ec);
};


template<class Buffer>
void Client::writeBuffer(Buffer& t_buffer)
{
    boost::asio::async_write(socket,
                             t_buffer,
                             [this](boost::system::error_code ec, size_t /*length*/)
                             {
                                 doWriteFile(ec);
                             });
}
