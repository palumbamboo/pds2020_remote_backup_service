//
// Created by Daniele Leto on 17/08/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <fstream>
#include "Message.h"

using boost::asio::ip::tcp;
enum {MaxLength = 40000};

class Client {
private:
    tcp::socket socket;
    tcp::resolver::results_type endpointIterator;

    std::array<char, MaxLength> m_buf;
    boost::asio::streambuf m_request;
    std::ifstream m_sourceFile;
    std::string m_path;
    MessageCommand m_command;

    std::string m_task;
    std::string m_clientId;
    bool m_response;


    template<class Buffer>
    void writeBuffer(Buffer& t_buffer);

public:

    Client(boost::asio::io_service& ioService,
           tcp::resolver::results_type endpointIterator,
           Message& message);

    ~Client();

    void call_connect();
    void openFile(Message& t_message);
    void openDeleteFile(Message& t_message);
    void doWriteFile(const boost::system::error_code& t_ec);
    void sendLoginRequest(Message& t_message);
    void sendInfoRequest(Message& t_message);


    void doRead();
    void processRead(size_t t_bytesTransferred);

    bool getResponse() { return m_response; }
};


template<class Buffer>
void Client::writeBuffer(Buffer& t_buffer)
{
    boost::asio::async_write(socket,
                             t_buffer,
                             [this](boost::system::error_code ec, size_t /*length*/)
                             {
                                std::cout << "Inside Write Buffer..................." << std::endl;
                                doWriteFile(ec);
                             });
}
