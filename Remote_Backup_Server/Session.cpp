//
// Created by Daniele Leto on 18/08/2020.
//

#include "Session.h"

Session::Session(tcp::socket socket) : socket{std::move(socket)} {}

void Session::start() {
    doRead();
}

void Session::doRead()
{
    auto self = shared_from_this();
    async_read_until(socket, m_requestBuf_, "\n\n",
                     [this, self](boost::system::error_code ec, size_t bytes)
                     {
                         if (!ec)
                             processRead(bytes);
                         else {
                             std::cout << ec << std::endl;
                             std::cout << ec.message() << std::endl;
                         }
                     });
}

int Session::createFile()
{
    std::filesystem::path total_filename;
    total_filename.append(m_clientId + "/" + std::string(m_fileName));
    std::cout << "total_fileName: " << total_filename << std::endl;
    std::filesystem::path root_name = total_filename;
    std::filesystem::create_directories(root_name.remove_filename());
    m_outputFile.open(total_filename, std::ios_base::binary);
    if (!m_outputFile) {
        std::cout <<  "Failed to create: " << total_filename << std::endl;
        std::flush(std::cout);
        return -1;
    }
    return 0;
}

void Session::doReadFileContent(size_t t_bytesTransferred)
{
    if (t_bytesTransferred > 0) {
        m_outputFile.write(m_buf.data(), static_cast<std::streamsize>(t_bytesTransferred));

        std::cout << " recv " << m_outputFile.tellp() << " bytes" << std::endl;

        if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
            std::cout << "\nReceived file: " << m_fileName << std::endl;
            return;
        }
    }
    auto self = shared_from_this();
    socket.async_read_some(boost::asio::buffer(m_buf.data(), m_buf.size()),
                             [this, self](boost::system::error_code ec, size_t bytes)
                             {
                                 doReadFileContent(bytes);
                             });
}

void Session::processRead(size_t t_bytesTransferred)
{
    std::istream requestStream(&m_requestBuf_);
    readData(requestStream);

    MessageCommand command = m_message.getCommand();

    if (command == MessageCommand::LOGIN_REQUEST) {
        // controlla se esiste la cartella, se non esiste la crea
        std::cout << m_clientId << std::endl;
        if (!std::filesystem::exists(m_clientId)) {
            if (std::filesystem::create_directories(m_clientId))
                std::cout << "directory: " << m_clientId << " correctly created!" << std::endl;
        } else {
            std::cout << "directory: " << m_clientId << " already exists!" << std::endl;
            std::cout << "User correctly logged!" << std::endl;
        }
        return;
    }

    if (command == MessageCommand::INFO_REQUEST) {
        // INFO_REQUEST | | clientID | | path | | filehashato
        std::cout << "INSIDE COMMAND = INFO_REQUEST " << std::endl;
        std::filesystem::path total_filename;
        total_filename.append(m_clientId + "/" + std::string(m_fileName));
        FileToUpload fileToUpload(total_filename);
        try {
            std::string hash = fileToUpload.fileHash();
            std::cout << "FILE path: " << m_fileName << " HASH: " << hash <<'\n';

            if(m_fileHash == hash)
                m_response = true;
            else
                m_response = false;
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
            m_response = false;
        }

        doWriteResponse();
        return;
    }

    if (command == MessageCommand::DELETE) {
        std::filesystem::path total_filename;
        total_filename.append(m_clientId + "/" + std::string(m_fileName));
        if (!std::filesystem::remove(total_filename)) {
            std::cout << "Error during removing.. " << total_filename << std::endl;
        } else
            std::cout << "Success! Removed.. " << total_filename << std::endl;
        return;
    }

    if (command == MessageCommand::CREATE) {
        if (createFile() == -1)
            return;

        // write extra bytes to file
        do {
            requestStream.read(m_buf.data(), m_buf.size());
            std::cout << "write " << requestStream.gcount() << " bytes.";
            m_outputFile.write(m_buf.data(), requestStream.gcount());
        } while (requestStream.gcount() > 0);

        auto self = shared_from_this();
        socket.async_read_some(boost::asio::buffer(m_buf.data(), m_buf.size()),
                               [this, self](boost::system::error_code ec, size_t bytes)
                               {
                                   if (!ec)
                                       doReadFileContent(bytes);
                                   else {
                                       std::cout << "ERRORE!" << std::endl;
                                       std::cout << ec.message() << std::endl;
                                   }
                               });
    }

}


void Session::readData(std::istream &stream)
{
    stream >> m_task;
    stream.read(m_buf.data(), 1);

    auto command = static_cast<MessageCommand>(stoi(m_task));

    if (command == MessageCommand::LOGIN_REQUEST) {
        // controlla se esiste la cartella, se non esiste la crea
        std::cout << "LOGIN" << std::endl;
        stream >> m_username;
        stream.read(m_buf.data(), 1);
        stream >> m_hashedPassword;
        m_message.setCommand(command);
        return;
    }

    stream >> m_clientId;
    stream.read(m_buf.data(), 1);

    // TODO: esiste il clientId? se no, errore, una directory per ogni client id e una cartella per ogni utente
    // TODO: info_request: se file esiste faccio hash con lo stesso metodo dell'altro e mando 1 se esiste e 0 se non esiste


    std::cout << "m_Task " << m_task << " m_clientID " << m_clientId << std::endl;

    if (command == MessageCommand::INFO_REQUEST) {
        // INFO_REQUEST | | clientID | | path | | filehashato
        std::cout << "INFO" << std::endl;
        stream >> m_fileName;
        stream.read(m_buf.data(), 1);
        stream >> m_fileHash;

        std::cout << "m_fileName " << m_fileName << std::endl;
        std::cout << "hash of file " << m_fileHash << std::endl;
        m_message.setCommand(command);
        m_message.setClientId(m_clientId);
        return;
    }

    if (command == MessageCommand::DELETE || command == MessageCommand::CREATE) {
        // CREATE or DELETE | | clientID | | path | | fileSize, if DELETE = 0
        stream >> m_fileName;
        stream.read(m_buf.data(), 1);
        stream >> m_fileSize;
        std::cout << "m_fileName " << m_fileName << " m_fileSize " << m_fileSize << std::endl;

        m_fileToUpload.setPath(m_clientId + "/" + std::string(m_fileName));
        m_fileToUpload.setFileSize(m_fileSize);

        m_message.setCommand(command);
        m_message.setFileToUpload(m_fileToUpload);
        m_message.setClientId(m_clientId);

        stream.read(m_buf.data(), 2);
        return;
    }
}

void Session::doWriteResponse() {
    if(m_message.getCommand() == MessageCommand::INFO_REQUEST) {
        std::ostream requestStream(&m_requestBuf_);
        m_message.setCommand(MessageCommand::INFO_RESPONSE);

        requestStream << static_cast<int>(m_message.getCommand()) << " " << m_message.getClientId() << " " << m_response << "\n\n";
        writeBuffer(m_requestBuf_);
    }
}

