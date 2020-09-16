//
// Created by Daniele Leto on 18/08/2020.
//

#include "Server.h"
#include "Session.h"

void Server::callAccept() {
    acceptor.async_accept(socket,
                          [this](boost::system::error_code ec) {
                              if (!ec) {
                                  std::make_shared<Session>(std::move(socket))->start();
                              }
                              callAccept();
                          });
}

Server::Server(boost::asio::io_service& ioService, short t_port) :
                socket{ioService}, acceptor{ioService, tcp::endpoint(tcp::v4(), t_port)} {
    std::cout << "Server started\n";
    //createLocalDirectory();

    callAccept();
}