//
// Created by daniele on 16/09/20.
//

#include "Message.h"

MessageCommand parseIntToCommand(int _command) {
    switch (_command) {
        case 0:
            return MessageCommand::CREATE;
        case 1:
            return MessageCommand::REMOVE;
        case 2:
            return MessageCommand::INFO_REQUEST;
        case 3:
            return MessageCommand::INFO_RESPONSE;
        case 4:
            return MessageCommand::END_INFO_PHASE;
        case 5:
            return MessageCommand::LOGIN_REQUEST;
        case 6:
            return MessageCommand::LOGIN_RESPONSE;
        default: {
            throw std::runtime_error("invalid command parsing");
        }
    }
}