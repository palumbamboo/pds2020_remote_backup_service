//
// Created by Daniele Leto on 17/08/2020.
//

#include <filesystem>
#include "Client.h"

bool is_number(const std::string& s);

Client::Client(const std::string &address, const std::string &port, Message & _message) :
        resolver{ioService},
        socket{ioService},
        message{_message} {
    endpointIterator = resolver.resolve(address, port);
}

Client::~Client() {
    socket.close();
}

void Client::start() {
    MessageCommand command = message.getCommand();

    switch (command) {
        // LOGIN_REQUEST | | username | | hashed password
        case MessageCommand::LOGIN_REQUEST: {
            m_command = MessageCommand::LOGIN_REQUEST;
            sendLoginRequest(message);
            break;
        }
            // INFO_REQUEST | | clientID | | path | | hashed file
        case MessageCommand::INFO_REQUEST: {
            m_command = MessageCommand::INFO_REQUEST;
            sendInfoRequest(message);
            break;
        }
            // END_INFO_PHASE | | clientID
        case MessageCommand::END_INFO_PHASE: {
            m_command = MessageCommand::END_INFO_PHASE;
            sendEndInfoRequest(message);
            break;
        }
            // REMOVE | | clientID | | path
        case MessageCommand::REMOVE: {
            m_command = MessageCommand::REMOVE;
            std::cout << "\tREMOVING file from server: " << message.getFile().getPathName() << std::endl;
            sendRemoveRequest(message);
            break;
        }
            // CREATE | | clientID | | path | | file size
            // file data inside request body
        case MessageCommand::CREATE: {
            m_command = MessageCommand::CREATE;
            std::cout << "\tSENDING file to server: " << message.getFile().getPathName() << ", size: " << message.getFile().getFileSize() << "B" << std::endl;
            openFile(message);
            break;
        }
        default: {
            std::cout << "\tERROR -> unknown command!" << std::endl;
            return;
        }
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
                                       if(_status == NOT_CONNECTED) {
                                           std::cout << "\tCONNECTION RESTORED" << std::endl;
                                           retry = 0;
                                           std::cout << "\tSENDING AGAIN file: " << message.getFile().getPathName() << std::endl;
                                       }
                                       _status = CONNECTED;
                                       writeBuffer(m_request);
                                   }
                                   else
                                   {
                                       _status = NOT_CONNECTED;
                                       retry++;
                                       std::cout << "\n\tCONNECTION ERROR -> error: " << ec.message() << ", retry #" << retry << std::endl;
                                       socket.close();
                                       if(retry > 10) {
                                           std::cout << "\tSERVER NOT ONLINE -> client execution stopped" << std::endl;
                                           throw std::runtime_error("SERVER NOT ONLINE");
                                       }
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

void Client::doWriteFile(const boost::system::error_code& t_ec)
{
    if (!t_ec) {
        if (m_sourceFile && m_command == MessageCommand::CREATE) {
            m_sourceFile.read(m_buf.data(), m_buf.size());
            if (m_sourceFile.fail() && !m_sourceFile.eof()) {
                auto msg = "Failed while reading file";
                std::cout << msg;
                throw std::fstream::failure(msg);
            }
            auto buf = boost::asio::buffer(m_buf.data(), static_cast<size_t>(m_sourceFile.gcount()));
            writeBuffer(buf);
            return;
        }

        doRead();
    } else {
        std::cout << "\n\tERROR -> with server connection: " << t_ec.message() << std::endl;
        m_response = false;
        return;
    }
}

void Client::sendLoginRequest(Message& t_message) {
    std::ostream requestStream(&m_request);

    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getUsername() << " "
                  << t_message.getPassword() << "\n\n";
}

void Client::sendInfoRequest(Message& t_message) {
    std::ostream requestStream(&m_request);

    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getClientId() << " "
                  << t_message.getFile().getPathToUpload() << " " << t_message.getFile().getFileStoredHash() << "\n\n";
}

void Client::sendEndInfoRequest(Message& t_message) {
    std::ostream requestStream(&m_request);

    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getClientId() << " " << t_message.getForceAlignment() << "\n\n";
}

void Client::sendRemoveRequest(Message& t_message) {
    std::string t_path = t_message.getFile().getPathToUpload();
    std::ostream requestStream(&m_request);

    requestStream << static_cast<int>(t_message.getCommand()) << " " << t_message.getClientId() << " " << t_message.getFile().getPathToUpload() << "\n\n";
}

void Client::processRead(size_t t_bytesTransferred) {
    std::istream requestStream(&m_request);
    // READ COMMAND TO EXECUTE
    requestStream >> m_task;
    if(m_task.size() != 1 && !is_number(m_task))
        throw std::runtime_error("invalid command parsing");
    requestStream.read(m_buf.data(), 1);

    auto command = parseIntToCommand(stoi(m_task));

    if (command == MessageCommand::INFO_RESPONSE    ||
        command == MessageCommand::END_INFO_PHASE   ||
        command == MessageCommand::CREATE  ||
        command == MessageCommand::REMOVE) {
        requestStream >> m_clientId;
        if (m_clientId.empty() || m_clientId.size()!=64 || m_clientId != message.getClientId())
            throw std::runtime_error("invalid command parsing");
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
                             std::cout << "\tERROR -> generic error with socket, connection aborted!" << std::endl;
                             return;
                         }
                     });
}

void Client::on_ready_to_reconnect(const boost::system::error_code &error) {
    try_connect();
}

std::string Client::getClientId() {
    return m_clientId;
}

bool is_number(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(),
                                      s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}