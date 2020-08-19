//
// Created by Daniele Leto on 18/08/2020.
//

#include <iostream>

#include <boost/asio/read_until.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include "Server.h"
#include "Session.h"

void Server::callAccept() {
    acceptor.async_accept(socket,
                          [this](boost::system::error_code ec)
                          {
                              if (!ec) {
                                  std::make_shared<Session>(std::move(socket))->start();
                              }

                              callAccept();
                          });
}

void Server::createRootDirectory()
{
    auto currentPath = std::filesystem::path(rootDirectory);
    if (!std::filesystem::exists(currentPath) && !std::filesystem::create_directory(currentPath))
        std::cerr << "Coudn't create local directory: " << currentPath;
    std::filesystem::current_path(currentPath);
    std::cout << "File system loaded\n";
}

Server::Server(boost::asio::io_service& ioService, short t_port, std::string  t_rootDirectory) :
        socket{ioService},
        acceptor{ioService, tcp::endpoint(tcp::v4(), t_port)},
        rootDirectory{std::move(t_rootDirectory)}
{
    createRootDirectory();
    std::cout << "Server started\n";

    callAccept();
}