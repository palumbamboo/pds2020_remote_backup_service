//
// Created by daniele on 16/09/20.
//

#ifndef REMOTE_BACKUP_CLIENT_UPLOADQUEUE_H
#define REMOTE_BACKUP_CLIENT_UPLOADQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>

class UploadQueue {
private:
    std::queue<std::string> queue;
    std::mutex mutex;
    std::condition_variable emptyQueue;
    std::condition_variable fullQueue;
    int size;
public:
    explicit UploadQueue(int _size) : size(_size), mutex() {}
    ~UploadQueue()=default;
    void pushMessage(const std::string & path);
    std::string popMessage();
};


#endif //REMOTE_BACKUP_CLIENT_UPLOADQUEUE_H
