//
// Created by Daniele Leto on 17/08/2020.
//

#include "Client.h"

Client::Client(boost::asio::io_service& ioService,
               tcp::resolver::results_type endpointIterator) :
               socket{ioService},
               endpointIterator{std::move(endpointIterator)} {
    call_connect();
    std::cout << "HERE" << std::endl;
    m_path = "../files_to_send/Sicurezza.zip";
    openFile(m_path);
}

Client::~Client() {
    std::cout << "Distructor called" << std::endl;
    socket.close();
}

void Client::call_connect() {
    boost::asio::async_connect(socket,
                               endpointIterator,
                               [this] (boost::system::error_code ec, const tcp::endpoint& endpoint) {
        if(!ec) {
            std::cout << "Connected" << std::endl;
            //std::string message = "Ciao \n20\n\n";
            writeBuffer(m_request);
        }
        else {
            std::cout << ec << std::endl;
            std::cout << "Coudn't connect to host. Please run server "
                         "or check network connection." << std::endl;
        }
    });
}
/*
void Client::write_buffer(std::string buf) {
    std::ostream request_stream(&request);
    request_stream << buf;
    std::cout << "Write buffer ->" << buf << std::endl;
    boost::asio::async_write(socket,
                             request,
                             [this] (boost::system::error_code ec, size_t) //length)
                             {
                                 if(!ec) {
                                     std::cout << "Fatto.." << std::endl;
                                     doWriteFile(ec);
                                 } else {
                                     std::cerr << "Errore.. " << ec << std::endl;
                                 }
                             });

}
*/

void Client::openFile(std::string const& t_path)
{
    m_sourceFile.open(t_path, std::ios_base::binary | std::ios_base::ate);
    if (m_sourceFile.fail())
        throw std::fstream::failure("Failed while opening file " + t_path);

    m_sourceFile.seekg(0, m_sourceFile.end);
    auto fileSize = m_sourceFile.tellg();
    m_sourceFile.seekg(0, m_sourceFile.beg);

    std::ostream requestStream(&m_request);
    std::filesystem::path p(t_path);
    std::cout << p.string() << std::endl;
    std::cout << "p.filename().string() :" << p.filename().string() << std::endl;
    requestStream << p.string() << "\n" << fileSize << "\n\n";
    std::cout << p.string() << "\n" << fileSize << "\n\n";
}

void Client::doWriteFile(const boost::system::error_code& t_ec)
{
    if (!t_ec) {
        if (m_sourceFile) {
            m_sourceFile.read(m_buf.data(), m_buf.size());
            if (m_sourceFile.fail() && !m_sourceFile.eof()) {
                auto msg = "Failed while reading file";
                std::cout << msg;
                throw std::fstream::failure(msg);
            }
            std::stringstream ss;
            ss << "Send " << m_sourceFile.gcount() << " bytes, total: "
               << m_sourceFile.tellg() << " bytes";
            std::cout << ss.str();
            std::cout << ss.str() << std::endl;

            auto buf = boost::asio::buffer(m_buf.data(), static_cast<size_t>(m_sourceFile.gcount()));
            writeBuffer(buf);
        }
    } else {
        std::cout << "Error: " << t_ec.message();
    }
}



