//
// Created by Daniele Leto on 18/08/2020.
//

#include "Session.h"

void scan_directory(const std::string& path_to_watch, std::vector<std::string> &_currentFiles);

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

void Session::processRead(size_t t_bytesTransferred)
{
    std::istream requestStream(&m_requestBuf_);
    readData(requestStream);

    MessageCommand command = m_message.getCommand();

    switch (command) {
        // LOGIN_REQUEST | | username | | hashed password
        case MessageCommand::LOGIN_REQUEST: {
            executeLoginCommand();
            break;
        }
            // INFO_REQUEST | | clientID | | path | | hashed file
        case MessageCommand::INFO_REQUEST: {
            executeInfoCommand();
            break;
        }
            // END_INFO_PHASE | | clientID
        case MessageCommand::END_INFO_PHASE: {
            executeEndInfoCommand();
            break;
        }
            // REMOVE | | clientID | | path
        case MessageCommand::REMOVE: {
            executeRemoveCommand();
            break;
        }
            // CREATE | | clientID | | path | | file size
            // file data inside request body
        case MessageCommand::CREATE: {
            executeCreateCommand(requestStream);
            break;
        }
        default: {
            std::cout << "\tERROR: unknown command!" << std::endl;
            std::terminate();       }
    }
}

void Session::readData(std::istream &stream) {
    try {
        // READ COMMAND TO EXECUTE
        stream >> m_task;
        stream.read(m_buf.data(), 1);

        auto command = parseIntToCommand(stoi(m_task));
        m_message.setCommand(command);

        if (command == MessageCommand::LOGIN_REQUEST) {
            stream >> m_username;
            stream.read(m_buf.data(), 1);
            stream >> m_hashedPassword;
            return;
        }

        // READ CLIENT ID IF NOT LOGIN COMMAND
        stream >> m_clientId;
        m_message.setClientId(m_clientId);
        stream.read(m_buf.data(), 1);

        if(command == MessageCommand::END_INFO_PHASE) {
            return;
        }

        stream >> m_fileName;
        stream.read(m_buf.data(), 1);

        if (command == MessageCommand::INFO_REQUEST) {
            stream >> m_fileHash;
            return;
        }

        if (command == MessageCommand::CREATE) {
            stream >> m_fileSize;

            m_fileToUpload.setPath(m_clientId + "/" + std::string(m_fileName));
            m_message.setFileToUpload(m_fileToUpload);
            m_fileToUpload.setFileSize(m_fileSize);
            return;
        }
    } catch (std::exception &e){
        std::cout << "\tERROR -> in message fields processing, abort" << std::endl;
        std::terminate();
    }
}

void Session::executeLoginCommand() {
    std::lock_guard<std::mutex> lg(mutex);

    auto it = userMap.find(m_username);
    if(it != userMap.end()) {
        std::string savedHashedPass = userMap[m_username][0];

        if(savedHashedPass == m_hashedPassword) {
            std::string savedClientID   = userMap[m_username][1];
            m_clientId = savedClientID;
            m_response = true;
            m_message.setClientId(m_clientId);
        } else {
            std::cout << "\tERROR -> The password is wrong -> ABORT!" << std::endl;
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

    if (m_response) {
        createClientFolder();
        std::cout << "User correctly logged!" << std::endl;
    }
}

void Session::executeInfoCommand() {
    std::filesystem::path total_filename(m_clientId + "/" + std::string(m_fileName));
    FileToUpload fileToUpload(total_filename);

    try {
        std::string hash = fileToUpload.fileHash();

        if(m_fileHash == hash) {
            std::lock_guard<std::mutex> lg(mutex);
            m_response = true;
            auto it = userFilesMap.find(m_clientId);
            if(it != userFilesMap.end()) {
                userFilesMap[m_clientId].push_back(total_filename);
            } else {
                std::vector<std::string> files {total_filename};
                userFilesMap.insert(std::pair<std::string, std::vector<std::string>>(m_clientId, files));
            }
        } else
            m_response = false;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        m_response = false;
    }

    doWriteResponse();
}

void Session::executeEndInfoCommand() {
    std::lock_guard<std::mutex> lg(mutex);

    // Prima controllo che la chiave del client esista e
    // se non ci sono file nel vettore, posso buttare la cartella corrente
    auto it = userFilesMap.find(m_clientId);
    if(it == userFilesMap.end() || userFilesMap[m_clientId].empty()) {
        std::filesystem::remove_all(m_clientId);
        createClientFolder();
        m_response = true;
        doWriteResponse();
        return;
    }

    // Prendo i file che ho in questo momento
    std::vector<std::string> currentFiles;
    scan_directory(m_clientId, currentFiles);

    // Se la currentFiles è vuota significa che non ho niente del client
    if(currentFiles.empty()) {
        m_response = true;
        doWriteResponse();
        return;
    }

    std::vector<std::string> filesToKeep = userFilesMap[m_clientId];
    std::sort(currentFiles.begin(), currentFiles.end());
    std::sort(filesToKeep.begin(), filesToKeep.end());
    std::vector<std::string> differences;
    std::set_difference(currentFiles.begin(), currentFiles.end(),
                        filesToKeep.begin(), filesToKeep.end(),
                        std::back_inserter(differences));

    int countError = 0;
    for (auto &file : differences) {
        if (!std::filesystem::remove(file)) {
            std::cout << "\tERROR deleting file " << file << " for client " << m_clientId << " alignment!" << std::endl;
            m_response = false;
            countError++;
        }
    }

    // check if some error in deleting files
    if (countError == 0)
        m_response = true;

    doWriteResponse();
}

void Session::executeRemoveCommand() {
    std::filesystem::path total_filename(m_clientId + "/" + std::string(m_fileName));
    // TODO : Check, removes but wrong message printed
    if (!std::filesystem::remove(total_filename)) {
        std::cout << "\tERROR deleting file " << m_fileName << " for client " << m_clientId << std::endl;
    }
}

void Session::executeCreateCommand(std::istream &requestStream) {
    if (createFile() == -1)
        return;

    do {
        requestStream.read(m_buf.data(), m_buf.size());
        m_outputFile.write(m_buf.data(), requestStream.gcount());
    } while (requestStream.gcount() > 0);

    auto self = shared_from_this();
    socket.async_read_some(boost::asio::buffer(m_buf.data(), m_buf.size()),
                           [this, self](boost::system::error_code ec, size_t bytes)
                           {
                               if (!ec)
                                   doReadFileContent(bytes);
                               else {
                                   std::cout << "\tERROR -> error reading from socket (client " << m_clientId << ") buffer: " << ec.message() << std::endl;
                               }
                           });
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

        std::cout << static_cast<int>(m_message.getCommand()) << " " << m_message.getClientId() << " " << m_response << "\n\n";
        requestStream << static_cast<int>(m_message.getCommand()) << " " << m_message.getClientId() << " " << m_response << "\n\n";
        writeBuffer(m_requestBuf_);
    }
}

int Session::createFile()
{
    std::filesystem::path total_filename;
    total_filename.append(m_clientId + "/" + std::string(m_fileName));
    std::filesystem::path root_name = total_filename;
    std::filesystem::create_directories(root_name.remove_filename());
    m_outputFile.open(total_filename, std::ios_base::binary);
    if (!m_outputFile) {
        std::cout <<  "\tERROR -> Failed to create path " << total_filename << std::endl;
        std::flush(std::cout);
        return -1;
    }
    return 0;
}

void Session::doReadFileContent(size_t t_bytesTransferred)
{
    if (t_bytesTransferred > 0) {
        m_outputFile.write(m_buf.data(), static_cast<std::streamsize>(t_bytesTransferred));

        if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
            std::cout << "\tReceived file: " << m_fileName << " from client " << m_clientId << std::endl;
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

void Session::createClientFolder() const {
    if (!std::filesystem::exists(m_clientId)) {
        if (std::filesystem::create_directories(m_clientId))
            std::cout << "directory: " << m_clientId << " correctly created!" << std::endl;
    } else {
        std::cout << "directory: " << m_clientId << " already exists!" << std::endl;
    }
}

void scan_directory(const std::string& path_to_watch, std::vector<std::string> &_currentFiles) {
    for(auto itEntry = std::filesystem::recursive_directory_iterator(path_to_watch);
        itEntry != std::filesystem::recursive_directory_iterator();
        ++itEntry ) {
        const auto filenameStr = itEntry->path().string();
        if (itEntry->is_regular_file()) {
            _currentFiles.push_back(filenameStr);
        }
    }
}

std::string Session::randomString(size_t length) {
    const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, charset.size() - 1);
    std::string random_string;

    for (std::size_t i = 0; i < length; ++i)
        random_string += charset[distribution(generator)];

    return random_string;
}




