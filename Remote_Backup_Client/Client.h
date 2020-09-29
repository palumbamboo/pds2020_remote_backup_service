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
enum ConnectionStatus{
    NOT_CONNECTED,
    CONNECTED
};

class Client {
private:
    boost::asio::io_service ioService;
    tcp::resolver resolver;
    tcp::socket socket;
    tcp::resolver::results_type endpointIterator;
    ::boost::asio::steady_timer m_timer{ioService, boost::asio::chrono::seconds{2}};

    std::array<char, MaxLength> m_buf;
    boost::asio::streambuf m_request;
    std::ifstream m_sourceFile;
    MessageCommand m_command;

    std::string m_task;
    std::string m_clientId;
    bool m_response;

    ConnectionStatus _status = NOT_CONNECTED;
    Message message;
    template<class Buffer>
    void writeBuffer(Buffer& t_buffer);

public:

    Client(const std::string &address, const std::string &port, Message & _message);

    ~Client();
    void start();
    void try_connect();
    void on_ready_to_reconnect(const boost::system::error_code &error);
    void openFile(Message& t_message);
    void openDeleteFile(Message& t_message);
    void doWriteFile(const boost::system::error_code& t_ec);
    void sendLoginRequest(Message& t_message);
    void sendInfoRequest(Message& t_message);
    void sendEndInfoPhase(Message& t_message);
    void doRead();
    void processRead(size_t t_bytesTransferred);
    bool getResponse() const { return m_response; }

    std::string getClientId();

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
