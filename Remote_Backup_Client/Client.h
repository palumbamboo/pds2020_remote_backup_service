//
// Created by Daniele Leto on 17/08/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class Client {
private:
    tcp::socket socket;
    tcp::resolver::results_type endpointIterator;

public:
    Client(boost::asio::io_service& ioService,
           tcp::resolver::results_type endpointIterator);
    void callConnect();
};
