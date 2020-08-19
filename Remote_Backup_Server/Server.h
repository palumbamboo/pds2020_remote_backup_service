//
// Created by Daniele Leto on 18/08/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <memory>

using boost::asio::ip::tcp;

class Server {
private:
    tcp::socket socket;
    tcp::acceptor acceptor;
    std::string rootDirectory;

public:
    Server(boost::asio::io_service& ioService, short t_port, std::string  t_rootDirectory);
    void callAccept();
    void createRootDirectory();

};
