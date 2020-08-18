//
// Created by Daniele Leto on 17/08/2020.
//

#include "Client.h"

Client::Client(boost::asio::io_service& ioService,
               boost::asio::ip::tcp::resolver::results_type endpointIterator) :
               socket(ioService),
               endpointIterator(std::move(endpointIterator)) {
    callConnect();
}

void Client::callConnect() {
    boost::asio::connect(socket, endpointIterator);
    boost::asio::async_connect(socket, endpointIterator,
                               [this](const boost::system::error_code& ec,
                                      const tcp::endpoint& endpoint) {
        if(!ec) {
            std::cout << "Connected" << std::endl;
            socket.async_read_some(boost::asio::buffer(buffer.data(), buffer.size()),
                                     [this](boost::system::error_code ec, size_t bytes)
                                     {
                                         if (!ec) {
                                             std::cout.write(buffer.data(), bytes);
                                         }
                                         else {
                                             std::cout << ec << std::endl;
                                         }
                                     });
        }
        else {
            std::cout << ec << std::endl;
            std::cout << "Coudn't connect to host. Please run server "
                         "or check network connection." << std::endl;
        }
    });
}

