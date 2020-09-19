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
    CREATE = 0,
    DELETE = 1,
    INFO_REQUEST = 2,
    INFO_RESPONSE = 3,
    LOGIN_REQUEST = 4,
    LOGIN_RESPONSE = 5,
};

class Message {
    FileToUpload fileToUpload;
    MessageCommand command;
    unsigned long clientId;
public:
    Message()=default;
    explicit Message(MessageCommand _command,
                     FileToUpload _fileToUpload,
                     unsigned long _clientId) :
            command(_command), fileToUpload(std::move(_fileToUpload)), clientId(_clientId) {}
    ~Message()=default;
    MessageCommand getCommand() { return command; }
    unsigned long getClientId() { return clientId; }
    FileToUpload getFile() { return fileToUpload; }

    int getCommandInt() { return static_cast<int>(command); }

    void setFileToUpload(const FileToUpload &_fileToUpload) {
        fileToUpload = _fileToUpload;
    }

    void setCommand(MessageCommand _command) {
        command = _command;
    }

    void setCommand(int _command) {
         command = static_cast<MessageCommand>(_command);
    }

    void setClientId(unsigned long _clientId) {
        clientId = _clientId;
    }

};

#endif //REMOTE_BACKUP_CLIENT_MESSAGE_H
