//
// Created by Daniele Leto on 17/08/2020.
//

#include <filesystem>
#include "Client.h"

Client::Client(const std::string &address, const std::string &port, Message & _message) :
        resolver{ioService},
        socket{ioService},
        message{_message}{
    endpointIterator = resolver.resolve(address, port);
}

Client::~Client() {
    socket.close();
}

void Client::start() {
    if(message.getCommand() == MessageCommand::CREATE) {
        openFile(message);
        std::cout << "\tSEND file to server: " << message.getFile().getPathName();
    } else if(message.getCommand() == MessageCommand::REMOVE){
        std::cout << "\tREMOVE file from server: " << message.getFile().getPathName();
        openDeleteFile(message);
    } else if(message.getCommand() == MessageCommand::LOGIN_REQUEST) {
        m_command = MessageCommand::LOGIN_REQUEST;
        sendLoginRequest(message);
    } else if(message.getCommand() == MessageCommand::INFO_REQUEST) {
        m_command = MessageCommand::INFO_REQUEST;
        sendInfoRequest(message);
    }
    try_connect();
    ioService.run();
}

void Client::try_connect() {
    boost::asio::async_connect(socket,
                               endpointIterator,
                               [this](boost::system::error_code ec, const tcp::endpoint &endpoint){
                                   if(!ec)
                                   {
                                       _status = CONNECTED;
                                       writeBuffer(m_request);
                                       if(this->message.getCommand() == MessageCommand::CREATE || this->message.getCommand() == MessageCommand::REMOVE)
                                           std::cout << " -> DONE" << std::endl;
                                   }
                                   else
                                   {
                                       _status = NOT_CONNECTED;
                                       std::cout << "\tCONNECTION ERROR -> code: " << ec << "\n\terror: " << ec.message()
                                                 << std::endl;
                                       socket.close();
                                       m_timer.expires_from_now(boost::asio::chrono::seconds{2});
                                       m_timer.async_wait(std::bind(&Client::on_ready_to_reconnect, this, std::placeholders::_1));
                                   }
                               });
}

void Client::openFile(Message& t_message)
{
    std::string t_path = t_message.getFile().getPath().string();

    m_sourceFile.open(t_path, std::ios_base::binary | std::ios_base::ate);
    if (m_sourceFile.fail())
        throw std::fstream::failure("Failed while opening file " + t_path + "\n");

    m_sourceFile.seekg(0, std::ifstream::end);
    auto fileSize = m_sourceFile.tellg();
    m_sourceFile.seekg(0, std::ifstream::beg);
    t_message.getFile().setFileSize(fileSize);

    std::ostream requestStream(&m_request);
    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getClientId() << " " << t_message.getFile().getPathToUpload() << " " << t_message.getFile().getFileSize() << "\n\n";
}

void Client::openDeleteFile(Message& t_message)
{
    std::string t_path = t_message.getFile().getPathToUpload();
    t_message.getFile().setFileSize(0);

    std::ostream requestStream(&m_request);
    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getClientId() << " " << t_message.getFile().getPathToUpload() << " " << t_message.getFile().getFileSize() << "\n\n";
}

void Client::doWriteFile(const boost::system::error_code& t_ec)
{
    if (!t_ec) {
        if (m_command == MessageCommand::INFO_REQUEST) {
            doRead();
            return;
        } else if (m_command == MessageCommand::LOGIN_REQUEST) {
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
            auto buf = boost::asio::buffer(m_buf.data(), static_cast<size_t>(m_sourceFile.gcount()));
            writeBuffer(buf);
        }
    } else {
        std::cout << "Error: " << t_ec.message();
    }
}

void Client::sendLoginRequest(Message& t_message) {
    std::ostream requestStream(&m_request);

    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getUsername() << " "
    << t_message.getPassword() << "\n\n";
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

    auto command = static_cast<MessageCommand>(stoi(m_task));
    if (command == MessageCommand::INFO_RESPONSE || command == MessageCommand::END_INFO_PHASE) {
        requestStream >> m_clientId;
        requestStream.read(m_buf.data(), 1);
        requestStream >> m_response;
    }

    if (command == MessageCommand::LOGIN_RESPONSE) {
        requestStream >> m_clientId;
        if (m_clientId == "0") {
            m_response = false;
        } else {
            m_response = true;
        }
    }
}

void Client::doRead() {
    async_read_until(socket, m_request, "\n\n",
                     [this](boost::system::error_code ec, size_t bytes) {
                         if (!ec)
                             processRead(bytes);
                         else {
                             std::cout << ec << std::endl;
                             std::cout << ec.message() << std::endl;
                         }
                     });
}

void Client::on_ready_to_reconnect(const boost::system::error_code &error) {
    try_connect();
}

std::string Client::getClientId() {
    return m_clientId;
}
