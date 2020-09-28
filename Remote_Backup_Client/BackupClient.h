//
// Created by daniele on 27/09/20.
//

#ifndef REMOTE_BACKUP_CLIENT_BACKUPCLIENT_H
#define REMOTE_BACKUP_CLIENT_BACKUPCLIENT_H

#include <utility>


#include "FileWatcher.h"
#include "UploadQueue.h"
#include "Client.h"

class BackupClient {
private:
    FileWatcher * fileWatcher;
    UploadQueue * uploadQueue;
    Client * currentClient;
public:
    BackupClient()=default;
    ~BackupClient()=default;
    void set_fileWatcher(FileWatcher &_fileWatcher) {
        fileWatcher = &_fileWatcher;
    }

    void set_uploadQueue(UploadQueue &_uploadQueue) {
        uploadQueue = &_uploadQueue;
    }

    FileWatcher* get_fileWatcher() {
        return fileWatcher;
    }

    UploadQueue* get_uploadQueue() {
        return uploadQueue;
    }

    void set_currentClient(Client &_currentClient) {
        currentClient = &_currentClient;
    }

    void delete_currentClient() {
        currentClient = nullptr;
    }

    Client* get_currentClient() {
        return currentClient;
    }

};


#endif //REMOTE_BACKUP_CLIENT_BACKUPCLIENT_H
