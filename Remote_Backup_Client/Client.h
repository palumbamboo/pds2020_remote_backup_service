//
// Created by Daniele Leto on 17/08/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
enum {MaxLength = 4096};

class Client {
private:
    tcp::socket socket;
    tcp::resolver::results_type endpointIterator;

    std::array<char, MaxLength> buffer;

public:

    Client(boost::asio::io_service& ioService,
           tcp::resolver::results_type endpointIterator);
    void call_connect();
    void write_buffer(std::string buffer);
};
