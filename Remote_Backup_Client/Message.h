//
// Created by daniele on 16/09/20.
//

#ifndef REMOTE_BACKUP_CLIENT_MESSAGE_H
#define REMOTE_BACKUP_CLIENT_MESSAGE_H

#include <string>
#include <filesystem>
#include <utility>
#include "FileToUpload.h"

enum class MessageCommand {CREATE, DELETE, CHECKSUM_REQUEST, CHECKSUM_RESPONSE};

class Message {
    FileToUpload fileToUpload;
    MessageCommand command;
    unsigned long clientId;
public:
    explicit Message(MessageCommand _command,
                     FileToUpload _fileToUpload,
                     unsigned long _clientId) :
            command(_command), fileToUpload(std::move(_fileToUpload)), clientId(_clientId) {}
    ~Message()=default;
    MessageCommand getCommand() { return command; }
    unsigned long getClientId() { return clientId; }
    FileToUpload getFile() { return fileToUpload; }
};

#endif //REMOTE_BACKUP_CLIENT_MESSAGE_H
