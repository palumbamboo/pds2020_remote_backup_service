//
// Created by Daniele Leto on 18/08/2020.
//

#include "Session.h"

Session::Session(tcp::socket socket) : socket{std::move(socket)} {}

void Session::start() {
    //TODO - why this is called two times?
    std::cout << "New session started.. " << std::endl;
    call_read();
}

void Session::call_read() {

    auto self(shared_from_this());
    socket.async_read_some(boost::asio::buffer(buffer,MaxLength),
                           [this, self] (boost::system::error_code ec, std::size_t length)
                           {
                                if(!ec) {
                                    std::cout.write(buffer.data(), length);
                                    call_write(length);
                                }
                           }
    );
}

void Session::call_write(std::size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(socket,boost::asio::buffer(buffer, length),
                        [this, self](boost::system::error_code ec, std::size_t length)
                        {
                            if(!ec) {
                                call_read();
                            }
                        }
    );
}
