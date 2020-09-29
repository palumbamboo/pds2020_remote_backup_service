//
// Created by Daniele Leto on 18/08/2020.
//

#include "Session.h"
#include <random>

std::string randomString(size_t length ) {
    {
        const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        std::random_device random_device;
        std::mt19937 generator(random_device());
        std::uniform_int_distribution<> distribution(0, charset.size() - 1);

        std::string random_string;

        for (std::size_t i = 0; i < length; ++i)
        {
            random_string += charset[distribution(generator)];
        }

        return random_string;
    }
}

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

void scan_directory(const std::string& path_to_watch, std::vector<std::string> &_currentFiles) {
    for(auto itEntry = std::filesystem::recursive_directory_iterator(path_to_watch);
        itEntry != std::filesystem::recursive_directory_iterator();
        ++itEntry ) {
        const auto filenameStr = itEntry->path().string();
        if (itEntry->is_regular_file()) {
            std::cout << "\tFILE: " << filenameStr << " added to current user file map" << std::endl;
            _currentFiles.push_back(filenameStr);
        } else if(itEntry->is_directory()){
            std::cout << "\tDIR: " << filenameStr << std::endl;
        } else
            std::cout << "??    " << filenameStr << std::endl;
    }
}

void Session::processRead(size_t t_bytesTransferred)
{
    std::istream requestStream(&m_requestBuf_);
    readData(requestStream);

    MessageCommand command = m_message.getCommand();

    if (command == MessageCommand::LOGIN_REQUEST) {
        // controlla se esiste la cartella, se non esiste la crea
        std::cout << m_hashedPassword << std::endl;

        std::lock_guard<std::mutex> lg(mutex);
        auto it = userMap.find(m_username);
        if(it != userMap.end()) {
            std::cout << "Lo username esiste" <<std::endl;
            std::string savedHashedPass = userMap[m_username][0];

            if(savedHashedPass == m_hashedPassword) {
                std::string savedClientID   = userMap[m_username][1];
                m_clientId = savedClientID;
                std::cout << m_clientId << std::endl;
                m_response = true;
                m_message.setClientId(m_clientId);
            } else {
                std::cout << "La password è sbagliata -> ABORT!" << std::endl;
                m_response = false;
            }
        } else {
            std::cout << "Lo username NON esiste" <<std::endl;
            std::string newClientID = randomString(64);
            std::vector<std::string> newUser{m_hashedPassword, newClientID};
            m_clientId = newClientID;
            m_message.setClientId(m_clientId);
            userMap.insert(std::pair<std::string, std::vector<std::string>>(m_username, newUser));
            m_response = true;
            nlohmann::json jsonMap(userMap);
            std::ofstream o(PASS_PATH);
            o << std::setw(4) << jsonMap << std::endl;
        }

        doWriteResponse();
        std::cout << m_clientId << std::endl;

        if (m_response) {
            if (!std::filesystem::exists(m_clientId)) {
                if (std::filesystem::create_directories(m_clientId))
                    std::cout << "directory: " << m_clientId << " correctly created!" << std::endl;
            } else {
                std::cout << "directory: " << m_clientId << " already exists!" << std::endl;
                std::cout << "User correctly logged!" << std::endl;
            }
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

            if(m_fileHash == hash) {
                m_response = true;
                std::lock_guard<std::mutex> lg(mutex);
                auto it = userFilesMap.find(m_clientId);
                if(it != userFilesMap.end()) {
                    std::cout << "clientID presente" << std::endl;
                    userFilesMap[m_clientId].push_back(total_filename);
                } else {
                    std::vector<std::string> files {total_filename};
                    userFilesMap.insert(std::pair<std::string, std::vector<std::string>>(m_clientId, files));
                    std::cout << "clientID wasn't present and i add it!" << std::endl;
                }
            } else
                m_response = false;
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
            m_response = false;
        }

        doWriteResponse();
        return;
    }

    if (command == MessageCommand::END_INFO_PHASE) {
        std::vector<std::string> currentFiles;
        scan_directory(m_clientId, currentFiles);
        for(auto &str : currentFiles)
            std::cout << str << std::endl;

        std::lock_guard<std::mutex> lg(mutex);
        std::vector<std::string> filesToKeep = userFilesMap[m_clientId];
        std::sort(currentFiles.begin(), currentFiles.end());
        std::sort(filesToKeep.begin(), filesToKeep.end());

        std::vector<std::string> differences;
        std::set_difference(currentFiles.begin(), currentFiles.end(),
                            filesToKeep.begin(), filesToKeep.end(),
                            std::back_inserter(differences));

        for (auto &i : differences) {
            std::cout << i << std::endl;
        }
        return;
    }

    if (command == MessageCommand::REMOVE) {
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

        std::cout << m_username << " " << m_hashedPassword << std::endl;
        return;
    }

    stream >> m_clientId;
    m_message.setClientId(m_clientId);
    stream.read(m_buf.data(), 1);

    if (command == MessageCommand::INFO_REQUEST) {
        // INFO_REQUEST | | clientID | | path | | filehashato
        std::cout << "INFO" << std::endl;
        stream >> m_fileName;
        stream.read(m_buf.data(), 1);
        stream >> m_fileHash;

        std::cout << "m_fileName " << m_fileName << std::endl;
        std::cout << "hash of file " << m_fileHash << std::endl;
        m_message.setCommand(command);
        return;
    }

    if (command == MessageCommand::END_INFO_PHASE) {
        std::cout << "END_INFO_PHASE" << std::endl;
        m_message.setCommand(command);
    }

    if (command == MessageCommand::REMOVE || command == MessageCommand::CREATE) {
        // CREATE or DELETE | | clientID | | path | | fileSize, if DELETE = 0
        stream >> m_fileName;
        stream.read(m_buf.data(), 1);
        stream >> m_fileSize;
        std::cout << "m_fileName " << m_fileName << " m_fileSize " << m_fileSize << std::endl;

        m_fileToUpload.setPath(m_clientId + "/" + std::string(m_fileName));
        m_fileToUpload.setFileSize(m_fileSize);

        m_message.setCommand(command);
        m_message.setFileToUpload(m_fileToUpload);

        //stream.read(m_buf.data(), 2);
        return;
    }
}

void Session::doWriteResponse() {
    if(m_message.getCommand() == MessageCommand::LOGIN_REQUEST) {
        std::ostream requestStream(&m_requestBuf_);
        m_message.setCommand(MessageCommand::LOGIN_RESPONSE);

        if(m_response) {
            requestStream << static_cast<int>(m_message.getCommand()) << " " << m_message.getClientId() << "\n\n";
        } else {
            requestStream << static_cast<int>(m_message.getCommand()) << " " << m_response << "\n\n";
        }
        writeBuffer(m_requestBuf_);
    }

    if(m_message.getCommand() == MessageCommand::INFO_REQUEST) {
        std::ostream requestStream(&m_requestBuf_);
        m_message.setCommand(MessageCommand::INFO_RESPONSE);

        requestStream << static_cast<int>(m_message.getCommand()) << " " << m_message.getClientId() << " " << m_response << "\n\n";
        writeBuffer(m_requestBuf_);
    }

    if(m_message.getCommand() == MessageCommand::END_INFO_PHASE) {
        std::ostream requestStream(&m_requestBuf_);
        m_message.setCommand(MessageCommand::END_INFO_PHASE);

        requestStream << static_cast<int>(m_message.getCommand()) << " " << m_message.getClientId() << " " << m_response << "\n\n";
        writeBuffer(m_requestBuf_);
    }
}

