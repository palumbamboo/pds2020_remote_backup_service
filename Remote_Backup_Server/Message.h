//
// Created by daniele on 16/09/20.
//

#ifndef REMOTE_BACKUP_CLIENT_MESSAGE_H
#define REMOTE_BACKUP_CLIENT_MESSAGE_H

#include <string>
#include <filesystem>
#include <utility>
#include "FileToUpload.h"

enum class MessageCommand {
    CREATE          = 0,
    REMOVE          = 1,
    INFO_REQUEST    = 2,
    INFO_RESPONSE   = 3,
    END_INFO_PHASE  = 4,
    LOGIN_REQUEST   = 5,
    LOGIN_RESPONSE  = 6,
};

MessageCommand parseIntToCommand(int _command);
std::string parseCommandToString(MessageCommand _command);

class Message {
    FileToUpload fileToUpload;
    MessageCommand command;
    std::string clientId;
public:
    Message()=default;
    explicit Message(MessageCommand _command,
                     FileToUpload& _fileToUpload,
                     std::string _clientId) :
            command(_command), fileToUpload(_fileToUpload), clientId(std::move(_clientId)) {}
    ~Message()=default;
    MessageCommand getCommand() { return command; }
    std::string getClientId() { return clientId; }
    FileToUpload& getFile() { return fileToUpload; }

    void setFileToUpload(const FileToUpload &_fileToUpload) {
        fileToUpload = _fileToUpload;
    }

    void setCommand(MessageCommand _command) {
        command = _command;
    }

    void setCommand(int _command) {
        command = static_cast<MessageCommand>(_command);
    }

    void setClientId(const std::string& _clientId) {
        clientId = _clientId;
    }
};

#endif //REMOTE_BACKUP_CLIENT_MESSAGE_H
