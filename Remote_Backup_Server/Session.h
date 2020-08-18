//
// Created by Daniele Leto on 18/08/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <filesystem>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
private:
    tcp::socket socket;
    enum {MaxLength = 4096};
    std::array<char, MaxLength> buffer;

    void call_read();
    void call_write(std::size_t length);

public:
    explicit Session(tcp::socket socket);

    void start();
};
