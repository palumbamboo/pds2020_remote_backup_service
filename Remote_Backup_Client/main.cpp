#include <iostream>
#include "FileWatcher.h"
#include "Client.h"

void runFileWatcher(std::string path_to_watch) {
    try {
        // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
        FileWatcher fw{path_to_watch, std::chrono::milliseconds(1000)};

        // Start monitoring a folder for changes and (in case of changes)
        // run a user provided lambda function
        fw.start([](std::string path_to_watch, FileStatus status) -> void {
            // Process only regular files, all other file types are ignored
            if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) &&
                status != FileStatus::erased) {
                return;
            }

            switch (status) {
                case FileStatus::created:
                    std::cout << "File created: " << path_to_watch << std::endl;
                    break;
                case FileStatus::modified:
                    std::cout << "File modified: " << path_to_watch << std::endl;
                    break;
                case FileStatus::erased:
                    std::cout << "File erased: " << path_to_watch << std::endl;
                    break;
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
    std::string path_to_watch = "./";

    //TODO: control the path, if not exists -> raise exception
    std::thread tfw(runFileWatcher, path_to_watch);
    tfw.detach();

    try {
        boost::asio::io_service ioService;
        boost::asio::ip::tcp::resolver resolver(ioService);
        boost::asio::ip::tcp::resolver::results_type endpointIterator = resolver.resolve(address, port);

        Client client(ioService, endpointIterator);

        ioService.run();

        for(;;) {

        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Exceptions!" << std::endl;
    }

    return 0;
}


