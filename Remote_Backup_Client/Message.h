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
    REMOVE = 1,
    INFO_REQUEST = 2,
    INFO_RESPONSE = 3,
    END_INFO_PHASE = 4,
    LOGIN_REQUEST = 5,
    LOGIN_RESPONSE = 6,
};

class Message {
    FileToUpload fileToUpload;
    MessageCommand command;
    std::string clientId;
    std::string username;
    std::string password;
    bool forceAlignment;
public:
    explicit Message(MessageCommand _command,
                     std::string _username,
                     std::string _password) :
            command(_command), username(std::move(_username)), password(std::move(_password)) {}
    explicit Message(MessageCommand _command,
                     FileToUpload& _fileToUpload,
                     std::string _clientId) :
            command(_command), fileToUpload(_fileToUpload), clientId(std::move(_clientId)) {}
    explicit Message(MessageCommand _command,
                     std::string _clientId,
                     bool _forceAlignment) :
            command(_command), clientId(std::move(_clientId)), forceAlignment(_forceAlignment) {}
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

    void setForceAlignment(bool _forceAlignment) {
        forceAlignment = _forceAlignment;
    }

    const std::string &getUsername() {
        return username;
    }

    const std::string &getPassword() {
        return password;
    }

    bool getForceAlignment() {
        return forceAlignment;
    }

    void setUsername(const std::string &_username) {
        username = _username;
    }

    void setPassword(const std::string &_password) {
        password = _password;
    }
};

#endif //REMOTE_BACKUP_CLIENT_MESSAGE_H
