//
// Created by daniele on 16/09/20.
//

#include "UploadQueue.h"

void UploadQueue::pushMessage(const std::string &path) {
    std::unique_lock<std::mutex> uniqueLock(mutex);
    fullQueue.wait(uniqueLock, [this](){ return queue.size()<size; });
    std::cout << "PUSH QUEUE path= " << path << std::endl;
    queue.push(path);
    emptyQueue.notify_one();
}

std::string UploadQueue::popMessage() {
    std::unique_lock<std::mutex> uniqueLock(mutex);
    emptyQueue.wait(uniqueLock, [this](){ return !queue.empty(); });
    std::string popped = queue.front();
    queue.pop();
    std::cout << "POP QUEUE path= " << popped << std::endl;
    fullQueue.notify_one();
    return popped;
}

