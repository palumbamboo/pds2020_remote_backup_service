//
// Created by Daniele Leto on 17/08/2020.
//

#include <filesystem>
#include "Client.h"

Client::Client(boost::asio::io_service& ioService,
               tcp::resolver::results_type endpointIterator,
               Message & message) :
        socket{ioService},
        endpointIterator{std::move(endpointIterator)} {
    if(message.getCommand() == MessageCommand::CREATE) {
        openFile(message);
    } else if(message.getCommand() == MessageCommand::DELETE){
        openDeleteFile(message);
    }
    call_connect();
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

void Client::openFile(Message& t_message)
{
    std::string t_path = t_message.getFile().getPath().string();
    std::cout << "t_path " << t_path << std::endl;

    m_sourceFile.open(t_path, std::ios_base::binary | std::ios_base::ate);
    if (m_sourceFile.fail())
        throw std::fstream::failure("Failed while opening file " + t_path + "\n");

    t_path.erase(remove_if(t_path.begin(), t_path.end(), isspace), t_path.end());
    std::cout << "t_path TRIMMED: " << t_path << std::endl;
    m_sourceFile.seekg(0, std::ifstream::end);
    auto fileSize = m_sourceFile.tellg();
    m_sourceFile.seekg(0, std::ifstream::beg);

    t_message.getFile().setFileSize(fileSize);
    std::cout << "SETTED FILESIZE " << fileSize << " " << t_message.getFile().getFileSize() << std::endl;
    /*
    std::ostream requestStream(&m_request);
    std::filesystem::path p(t_path);
    std::cout << p.string() << std::endl;
    std::cout << "p.filename().string() :" << p.filename().string() << std::endl;
    requestStream << "GET " << p.string() << "\n" << fileSize << "\n\n";
    std::cout << "GET " << p.string() << "\n" << fileSize << "\n\n";
     */
    std::ostream requestStream(&m_request);
    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getClientId() << " " << t_message.getFile().getPath() << " " << t_message.getFile().getFileSize() << "\n\n";
}

void Client::openDeleteFile(Message& t_message)
{
    std::string t_path = t_message.getFile().getPath().string();
    std::cout << "t_path " << t_path << std::endl;
    t_path.erase(remove_if(t_path.begin(), t_path.end(), isspace), t_path.end());
    std::cout << "t_path TRIMMED: " << t_path << std::endl;
    /*
    std::ostream requestStream(&m_request);
    std::filesystem::path p(t_path);
    std::cout << p.string() << std::endl;
    std::cout << "p.filename().string() :" << p.filename().string() << std::endl;
    requestStream << "DEL " << p.string() << "\n" << 0 << "\n\n";
    std::cout << "DEL " << p.string() << "\n" << 0 << "\n\n";
    */
    std::ostream requestStream(&m_request);
    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getClientId() << " " << t_message.getFile().getPath() << " " << t_message.getFile().getFileSize() << "\n\n";
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
