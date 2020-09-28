//
// Created by Daniele Leto on 18/08/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <filesystem>

using boost::asio::ip::tcp;

class Server {
private:
    tcp::socket socket;
    tcp::acceptor acceptor;

public:
    Server(boost::asio::io_service& ioService, const std::string& t_port);
    void callAccept();
};
