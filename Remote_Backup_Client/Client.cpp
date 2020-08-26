//
// Created by Daniele Leto on 17/08/2020.
//

#include "Client.h"

Client::Client(boost::asio::io_service& ioService,
               tcp::resolver::results_type endpointIterator) :
               socket{ioService},
               endpointIterator{std::move(endpointIterator)} {
    call_connect();
}

Client::~Client() {
    std::cout << "Distructor called" << std::endl;
    socket.close();
}

void Client::call_connect() {
    boost::asio::connect(socket, endpointIterator);
    boost::asio::async_connect(socket,
                               endpointIterator,
                               [this] (boost::system::error_code ec, const tcp::endpoint& endpoint) {
        if(!ec) {
            std::cout << "Connected" << std::endl;
            std::string message = "Ciao\n";
            write_buffer(message);
        }
        else {
            std::cout << ec << std::endl;
            std::cout << "Coudn't connect to host. Please run server "
                         "or check network connection." << std::endl;
        }
    });
}

void Client::write_buffer(std::string buf) {
    auto bufs = boost::asio::buffer(buf);
    boost::asio::async_write(socket,
                             bufs,
                             [this] (boost::system::error_code ec, size_t /*length*/)
                             {
                                 if(!ec) {
                                     std::cout << "Fatto.." << std::endl;
                                     //call_write_file(ec);
                                 } else {
                                     std::cerr << "Errore.. " << ec << std::endl;
                                 }
                             });

}

/*
void Client::call_write_file(const boost::system::error_code& ec)
{
    if (!ec) {
        if (m_sourceFile) {
            m_sourceFile.read(m_buf.data(), m_buf.size());
            if (m_sourceFile.fail() && !m_sourceFile.eof()) {
                auto msg = "Failed while reading file";
                BOOST_LOG_TRIVIAL(error) << msg;
                throw std::fstream::failure(msg);
            }
            std::stringstream ss;
            ss << "Send " << m_sourceFile.gcount() << " bytes, total: "
               << m_sourceFile.tellg() << " bytes";
            BOOST_LOG_TRIVIAL(trace) << ss.str();
            std::cout << ss.str() << std::endl;

            auto buf = boost::asio::buffer(m_buf.data(), static_cast<size_t>(m_sourceFile.gcount()));
            writeBuffer(buf);
        }
    } else {
        BOOST_LOG_TRIVIAL(error) << "Error: " << t_ec.message();
    }
}

*/
