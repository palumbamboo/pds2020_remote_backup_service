//
// Created by Daniele Leto on 17/08/2020.
//

#include "Client.h"

Client::Client(boost::asio::io_service& ioService,
               boost::asio::ip::tcp::resolver::results_type endpointIterator) :
               socket(ioService),
               endpointIterator(std::move(endpointIterator)) {
//callConnect();
}

/*
void Client::callConnect() {
    boost::asio::connect(socket, endpointIterator);
    boost::asio::async_connect(socket, endpointIterator,
                               [this](boost::system::error_code ec)
                               {
                                   if (!ec) {
                                       //writeBuffer(m_request);
                                   } else {
                                       std::cout << "Coudn't connect to host. Please run server "
                                                    "or check network connection.\n";
                                       //BOOST_LOG_TRIVIAL(error) << "Error: " << ec.message();
                                   }
                               });

    std::cout << "Ciao" << std::endl;
}
 */

