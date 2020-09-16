//
// Created by Daniele Leto on 17/08/2020.
//

#include "Client.h"

std::mutex mutex;
std::queue<std::string> path_to_send;
std::condition_variable cv;

Client::Client(boost::asio::io_service& ioService,
               tcp::resolver::results_type endpointIterator,
               bool erase) :
               socket{ioService},
               endpointIterator{std::move(endpointIterator)} {
    if (!erase) {
        call_connect();
        std::cout << "HERE" << std::endl;
        std::unique_lock<std::mutex> ul(mutex);
        cv.wait(ul, []() { return !path_to_send.empty(); });
        if (!path_to_send.empty()) {
            std::string next_path = path_to_send.front();
            path_to_send.pop();
            openFile(next_path);
        }
    } else {
        call_connect_erase();

        std::unique_lock<std::mutex> ul(mutex);
        cv.wait(ul, []() { return !path_to_send.empty(); });
        if (!path_to_send.empty()) {
            std::string next_path = path_to_send.front();
            path_to_send.pop();
            std::ostream requestStream(&m_request);
            std::string p(next_path);
            p.erase(remove_if(p.begin(), p.end(), isspace), p.end());
            std::cout << "t_path TRIMMED: " << p << std::endl;
            std::cout << p << std::endl;
            requestStream << "DEL " << p << "\n" << 0 << "\n\n";
            std::cout << "DEL " << p << "\n" << 0 << "\n\n";
        }
    }
}

Client::~Client() {
    std::cout << "Distructor called" << std::endl;
    socket.close();
}

void Client::call_connect() {
    std::cout << "call_connect" << std::endl;
    boost::asio::async_connect(socket,
                               endpointIterator,
                               [this] (boost::system::error_code ec, const tcp::endpoint& endpoint)
                               {
                                if(!ec) {
                                    std::cout << "Connected" << std::endl;
                                    writeBuffer(m_request);
                                }
                                else {
                                    std::cout << ec << std::endl;
                                    std::cout << "Coudn't connect to host. Please run server "
                                                 "or check network connection." << std::endl;
                                }
    });
}

void Client::call_connect_erase() {
    std::cout << "call_connect ERASE" << std::endl;
    boost::asio::async_connect(socket,
                               endpointIterator,
                               [this] (boost::system::error_code ec, const tcp::endpoint& endpoint)
                               {
                                   if(!ec) {
                                       std::cout << "Connected" << std::endl;
                                       writeBufferErase(m_request);
                                   }
                                   else {
                                       std::cout << ec << std::endl;
                                       std::cout << "Coudn't connect to host. Please run server "
                                                    "or check network connection." << std::endl;
                                   }
                               });
}

void Client::openFile(std::string& t_path)
{
    std::cout << "t_path " << t_path << std::endl;

    m_sourceFile.open(t_path, std::ios_base::binary | std::ios_base::ate);
    if (m_sourceFile.fail())
        throw std::fstream::failure("Failed while opening file " + t_path + "\n");

    t_path.erase(remove_if(t_path.begin(), t_path.end(), isspace), t_path.end());
    std::cout << "t_path TRIMMED: " << t_path << std::endl;
    m_sourceFile.seekg(0, m_sourceFile.end);
    auto fileSize = m_sourceFile.tellg();
    m_sourceFile.seekg(0, m_sourceFile.beg);

    std::ostream requestStream(&m_request);
    std::filesystem::path p(t_path);
    std::cout << p.string() << std::endl;
    std::cout << "p.filename().string() :" << p.filename().string() << std::endl;
    requestStream << "GET " << p.string() << "\n" << fileSize << "\n\n";
    std::cout << "GET " << p.string() << "\n" << fileSize << "\n\n";
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

void Client::eraseFile(const boost::system::error_code& t_ec) {

}



