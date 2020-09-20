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
                             std::cout << "HERE" << std::endl;
                             std::cout << ec << std::endl;
                             std::cout << ec.message() << std::endl;
                         }
                     });
}

int Session::createFile()
{
    std::filesystem::path root_name = m_fileName;
    root_name.remove_filename();
    std::cout << "m_fileName: " << m_fileName << std::endl;
    std::cout << "root_name: " << root_name << std::endl;
    m_fileName.filename();
    std::cout << "m_fileName after: " << m_fileName << std::endl;
    std::filesystem::create_directories(root_name);
    m_outputFile.open(m_fileName, std::ios_base::binary);
    if (!m_outputFile) {
        std::cout <<  "Failed to create: " << m_fileName << std::endl;
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

    }

    if (command == MessageCommand::INFO_REQUEST) {
        // INFO_REQUEST | | clientID | | path | | filehashato

    }

    if (command == MessageCommand::DELETE) {
        if (!std::filesystem::remove(m_fileName)) {
            std::cout << "Error during removing.. " << m_fileName << std::endl;
        } else
            std::cout << "Success! Removed.. " << m_fileName << std::endl;
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
    stream >> m_clientId;
    stream.read(m_buf.data(), 1);

    // TODO: esiste il clientId? se no, errore, una directory per ogni client id e una cartella per ogni utente
    // TODO: info_request: se file esiste faccio hash con lo stesso metodo dell'altro e mando 1 se esiste e 0 se non esiste

    auto command = static_cast<MessageCommand>(stoi(m_task));

    std::cout << "m_Task " << m_task << " m_clientID " << m_clientId << std::endl;

    if (command == MessageCommand::LOGIN_REQUEST) {
        // controlla se esiste la cartella, se non esiste la crea
        std::cout << "LOGIN" << std::endl;
        stream >> hashed_password;
        stream.read(m_buf.data(), 1);
    }

    if (command == MessageCommand::INFO_REQUEST) {
        // INFO_REQUEST | | clientID | | path | | filehashato
        std::cout << "INFO" << std::endl;
        stream >> m_fileName;
        // add file hashed da confrontare con quello che calcolo io
    }

    if (command == MessageCommand::DELETE || command == MessageCommand::CREATE) {
        // CREATE or DELETE | | clientID | | path | | fileSize, if DELETE = 0
        stream >> m_fileName;
        stream.read(m_buf.data(), 1);
        stream >> m_fileSize;
        std::cout << "m_fileName " << m_fileName << " m_fileSize " << m_fileSize << std::endl;

        m_fileToUpload.setPath(m_fileName);
        m_fileToUpload.setFileSize(m_fileSize);

        m_message.setCommand(command);
        m_message.setFileToUpload(m_fileToUpload);
        m_message.setClientId(m_clientId);

        stream.read(m_buf.data(), 2);
        return;
    }
}

