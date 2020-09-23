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
    } else if(message.getCommand() == MessageCommand::LOGIN_REQUEST) {
        sendLoginRequest(message);
    } else if(message.getCommand() == MessageCommand::INFO_REQUEST) {
        m_command = MessageCommand::INFO_REQUEST;
        sendInfoRequest(message);
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

    m_sourceFile.seekg(0, std::ifstream::end);
    auto fileSize = m_sourceFile.tellg();
    m_sourceFile.seekg(0, std::ifstream::beg);
    t_message.getFile().setFileSize(fileSize);

    std::ostream requestStream(&m_request);
    std::cout << t_message.getClientId() << std::endl;
    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getClientId() << " " << t_message.getFile().getPathToUpload() << " " << t_message.getFile().getFileSize() << "\n\n";
}

void Client::openDeleteFile(Message& t_message)
{
    std::string t_path = t_message.getFile().getPathToUpload();
    std::cout << "t_path " << t_path << std::endl;
    t_message.getFile().setFileSize(0);

    std::ostream requestStream(&m_request);
    std::cout << t_message.getClientId() << std::endl;
    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getClientId() << " " << t_message.getFile().getPathToUpload() << " " << t_message.getFile().getFileSize() << "\n\n";
}

void Client::doWriteFile(const boost::system::error_code& t_ec)
{
    if (!t_ec) {
        if (m_command == MessageCommand::INFO_REQUEST) {
            doRead();
            return;
        }
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

void Client::sendLoginRequest(Message& t_message) {
    std::ostream requestStream(&m_request);

    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getClientId() << "\n\n";
}

void Client::sendInfoRequest(Message& t_message) {
    std::ostream requestStream(&m_request);

    // TODO: send correct infos
    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getClientId() << " "
        << t_message.getFile().getPathToUpload() << " " << t_message.getFile().getFileStoredHash() << "\n\n";
}

void Client::processRead(size_t t_bytesTransferred) {
    std::istream requestStream(&m_request);
    requestStream >> m_task;
    requestStream.read(m_buf.data(), 1);
    requestStream >> m_clientId;
    requestStream.read(m_buf.data(), 1);
    requestStream >> m_response;


    std::cout << m_task << " " << m_clientId << " " << m_response << std::endl;
}

void Client::doRead()
{
    async_read_until(socket, m_request, "\n\n",
                     [this](boost::system::error_code ec, size_t bytes)
                     {
                         if (!ec)
                             processRead(bytes);
                         else {
                             std::cout << ec << std::endl;
                             std::cout << ec.message() << std::endl;
                         }
                     });
}
