//
// Created by daniele on 16/09/20.
//

#include "Message.h"

MessageCommand parseIntToCommand(int _command) {
    switch (_command) {
        case 0:
            return MessageCommand::CREATE;
        case 1:
            return MessageCommand::CREATE_RESPONSE;
        case 2:
            return MessageCommand::REMOVE;
        case 3:
            return MessageCommand::REMOVE_RESPONSE;
        case 4:
            return MessageCommand::INFO_REQUEST;
        case 5:
            return MessageCommand::INFO_RESPONSE;
        case 6:
            return MessageCommand::END_INFO_PHASE;
        case 7:
            return MessageCommand::LOGIN_REQUEST;
        case 8:
            return MessageCommand::LOGIN_RESPONSE;
        default: {
            throw std::runtime_error("invalid command parsing");
        }
    }
}

std::string parseCommandToString(MessageCommand _command) {
    switch (_command) {
        case MessageCommand::CREATE:
            return "CREATE";
        case MessageCommand::REMOVE:
            return "REMOVE";
        case MessageCommand::INFO_REQUEST:
            return "INFO REQUEST";
        case MessageCommand::INFO_RESPONSE:
            return "INFO RESPONSE";
        case MessageCommand::END_INFO_PHASE:
            return "END INFO PHASE";
        case MessageCommand::LOGIN_REQUEST:
            return "LOGIN REQUEST";
        case MessageCommand::LOGIN_RESPONSE:
            return "LOGIN RESPONSE";
        default: {
            throw std::runtime_error("invalid command parsing");
        }
    }
}
