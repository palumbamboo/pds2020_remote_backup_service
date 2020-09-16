#include <iostream>
#include "FileWatcher.h"
#include "Client.h"
#include "UploadQueue.h"
#include "Message.h"
#include <boost/uuid/detail/md5.hpp>

#define CLIENTID 1234567890

std::string computeFileHash(const std::filesystem::recursive_directory_iterator &itEntry);

void createClientSend(Message& message, const std::string& address, const std::string& port) {
    boost::asio::io_service ioService;
    tcp::resolver resolver(ioService);
    tcp::resolver::results_type endpointIterator = resolver.resolve(address, port);

    Client client(ioService, endpointIterator, message);

    // ioService.run() continue till there are not asynchronous call opened.
    ioService.run();
}

void run_file_watcher(const std::string & path_to_watch, UploadQueue& queue) {
    try {
        // Create a FileWatcher instance that will check the current folder for changes every 1 second
        FileWatcher fw{path_to_watch, std::chrono::milliseconds(1000)};

        // Start monitoring a folder for changes and (in case of changes)
        // run a user provided lambda function
        fw.start([&](const std::string & path_to_watch, FileStatus status) -> void {
            // Process only regular files, all other file types are ignored
            std::filesystem::path filePath(path_to_watch);
            if (!std::filesystem::is_regular_file(filePath) && status != FileStatus::erased) {
                return;
            }

            switch (status) {
                case FileStatus::created: {
                    std::cout << "File created: " << path_to_watch << std::endl;
                    Message message(MessageCommand::CREATE, filePath, CLIENTID);
                    queue.pushMessage(message);
                    break;
                }
                case FileStatus::modified: {
                    std::cout << "File modified: " << path_to_watch << std::endl;
                    Message message(MessageCommand::CREATE, filePath, CLIENTID);
                    queue.pushMessage(message);
                    break;
                }
                case FileStatus::erased: {
                    std::cout << "File erased: " << path_to_watch << std::endl;
                    Message message(MessageCommand::DELETE, filePath, CLIENTID);
                    queue.pushMessage(message);
                    break;
                }
                default:
                    std::cout << "Error! Unknown file status." << std::endl;
            }
        });
    }
    catch (std::exception &e) {
        std::cout << "Exception!" << std::endl;
        std::cout << e.what() << std::endl;
    }
}

void scan_directory(std::string path_to_watch, UploadQueue& queue) {
    for(auto itEntry = std::filesystem::recursive_directory_iterator(path_to_watch);
        itEntry != std::filesystem::recursive_directory_iterator();
        ++itEntry ) {
        const auto filenameStr = itEntry->path().filename().string();
        std::cout << filenameStr << std::endl;
        std::cout << std::setw(itEntry.depth()*3) << "";
        if (itEntry->is_directory()) {
            std::cout << "dir:  " << filenameStr << '\n';
        }
        else if (itEntry->is_regular_file()) {
            std::cout << "file: " << filenameStr << '\n';
            std::cout << "files to send: itEntry " << itEntry->path() << std::endl;
            std::string hash = computeFileHash(itEntry);

            // todo - implementare invio al server del checksum
            if(checksum diverso) {
                Message message(MessageCommand::CREATE, itEntry->path(), CLIENTID);
                queue.pushMessage(message);
            }
        }
        else
            std::cout << "??    " << filenameStr << '\n';
    }
}

std::string computeFileHash(const std::filesystem::recursive_directory_iterator &itEntry) {
    std::ifstream file;
    file.open(itEntry->path(), std::ios_base::binary | std::ios_base::ate);
    if (file.fail())
        throw std::fstream::failure("Failed while opening file " + itEntry->path().string() + "\n");

    std::string s;
    while(std::getline(file, s)) {
        boost::uuids::detail::md5 hash;
        hash.process_bytes(s.data(), s.size());
    }
    return s;
}


int main(int argc, char* argv[]) {
    /*
    if (argc != 3) {
        std::cerr << "Usage: client <address> <port>\n";
        return 1;
    }

    auto address = argv[1];
    auto port = argv[2];
    */

    auto address = "localhost";
    //watch the port
    auto port = "2000";

    std::string path_to_watch = "../files_to_send";
    UploadQueue uploadQueue(100);
    bool shutdown = false;

    try {

        //TODO: control the path, if not exists -> raise exception
        std::thread tfw([&path_to_watch, &uploadQueue](){
            scan_directory(path_to_watch, uploadQueue);
            run_file_watcher(path_to_watch, uploadQueue);
        });

        std::thread tcq([&uploadQueue, &shutdown, &address, &port](){
            while(!shutdown) {
                Message message = uploadQueue.popMessage();
                createClientSend(message, address, port);
            }
        });

        tfw.join();
        tcq.join();
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Exceptions!" << std::endl;
    }

    return 0;
}



