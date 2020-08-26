//
// Created by Daniele Leto on 17/08/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
enum {MaxLength = 1024};

class Client {
private:
    tcp::socket socket;
    tcp::resolver::results_type endpointIterator;

    std::array<char, MaxLength> buffer;
    boost::asio::streambuf request;

public:

    Client(boost::asio::io_service& ioService,
           tcp::resolver::results_type endpointIterator);
    void call_connect();
    void write_buffer(std::string buf);
    ~Client();
    //void call_write_file(const boost::system::error_code& ec);
};