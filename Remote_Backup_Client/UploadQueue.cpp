//
// Created by daniele on 16/09/20.
//

#include "UploadQueue.h"

void UploadQueue::pushMessage(Message& message) {
    std::unique_lock<std::mutex> uniqueLock(mutex);
    fullQueue.wait(uniqueLock, [this](){ return queue.size()<size; });
    queue.push(message);
    emptyQueue.notify_one();
}

Message UploadQueue::popMessage() {
    std::unique_lock<std::mutex> uniqueLock(mutex);
    emptyQueue.wait(uniqueLock, [this](){ return !queue.empty(); });
    Message popped = queue.front();
    queue.pop();
    fullQueue.notify_one();
    if(queue.empty())
        destroyQueue.notify_one();
    return popped;
}

bool UploadQueue::readyToClose() {
    std::unique_lock<std::mutex> uniqueLock(mutex);
    if(!queue.empty())
        std::cout << "\tUPLOAD QUEUE NOT EMPTY -> waiting " << queue.size() << " messages" <<std::endl;
    destroyQueue.wait(uniqueLock, [this](){ return queue.empty(); });
    std::cout << "\tUPLOAD QUEUE -> empty" << std::endl;
    return true;
}

int UploadQueue::queueSize() {
    std::lock_guard<std::mutex> lockGuard(mutex);
    return queue.size();
}
