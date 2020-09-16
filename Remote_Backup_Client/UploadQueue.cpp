//
// Created by daniele on 16/09/20.
//

#include "UploadQueue.h"

void UploadQueue::pushMessage(Message& message) {
    std::unique_lock<std::mutex> uniqueLock(mutex);
    fullQueue.wait(uniqueLock, [this](){ return queue.size()<size; });
    std::cout << "PUSH QUEUE path= " << message.getPathName() << std::endl;
    queue.push(message);
    emptyQueue.notify_one();
}

Message UploadQueue::popMessage() {
    std::unique_lock<std::mutex> uniqueLock(mutex);
    emptyQueue.wait(uniqueLock, [this](){ return !queue.empty(); });
    Message popped = queue.front();
    queue.pop();
    std::cout << "POP QUEUE path= " << popped.getPathName() << std::endl;
    fullQueue.notify_one();
    return popped;
}

