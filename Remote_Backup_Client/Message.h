//
// Created by daniele on 16/09/20.
//

#ifndef REMOTE_BACKUP_CLIENT_MESSAGE_H
#define REMOTE_BACKUP_CLIENT_MESSAGE_H

#include <string>
#include <filesystem>
#include <utility>

enum class MessageCommand {CREATE, DELETE};

class Message {
    std::filesystem::path path;
    MessageCommand command;
    unsigned long clientId;
public:
    explicit Message(MessageCommand _command,
                     std::filesystem::path _path,
                     unsigned long _clientId) :
            command(_command), path(std::move(_path)), clientId(_clientId) {}
    ~Message()=default;
    MessageCommand getCommand() { return command; }
    std::filesystem::path getPath() { return path; }
    unsigned long getClientId() { return clientId; }
    std::string getPathName() { return path.filename().string(); }
};


#endif //REMOTE_BACKUP_CLIENT_MESSAGE_H
