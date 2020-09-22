#include <iostream>
#include <boost/program_options.hpp>
#include <random>
#include <functional>
#include <algorithm>
#include "FileWatcher.h"
#include "Client.h"
#include "UploadQueue.h"
#include "Message.h"
#include "FileToUpload.h"

#define VERSION "0.1"
#define CONFIG_PATH "remote_client.cfg"

std::string globalClientId;
std::string folderToWatch;

std::string randomString( size_t length ) {
    auto randomString = []() -> char
    {
        const char charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randomString);
    return str;
}

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
                    FileToUpload fileToUpload(folderToWatch, filePath);
                    Message message(MessageCommand::CREATE, fileToUpload, globalClientId);
                    queue.pushMessage(message);
                    break;
                }
                case FileStatus::modified: {
                    std::cout << "File modified: " << path_to_watch << std::endl;
                    FileToUpload fileToUpload(folderToWatch, filePath);
                    Message message(MessageCommand::CREATE, fileToUpload, globalClientId);
                    queue.pushMessage(message);
                    break;
                }
                case FileStatus::erased: {
                    std::cout << "File erased: " << path_to_watch << std::endl;
                    FileToUpload fileToUpload(folderToWatch, filePath);
                    Message message(MessageCommand::DELETE, fileToUpload, globalClientId);
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

void scan_directory(const std::string& path_to_watch, UploadQueue& queue) {
    for(auto itEntry = std::filesystem::recursive_directory_iterator(path_to_watch);
        itEntry != std::filesystem::recursive_directory_iterator();
        ++itEntry ) {
        const auto filenameStr = itEntry->path().filename().string();
        std::cout << std::setw(itEntry.depth()*3) << "";
        if (itEntry->is_directory()) {
            std::cout << "dir:  " << filenameStr << '\n';
        }
        else if (itEntry->is_regular_file()) {
            FileToUpload fileToUpload(folderToWatch, itEntry->path());
            std::string hash = fileToUpload.fileHash();
            std::cout << "FILE path: " << itEntry->path().filename() << " HASH: " << hash <<'\n';

            // todo - implementare invio al server del checksum
//          if(checksum diverso) {
            Message message(MessageCommand::CREATE, fileToUpload, globalClientId);
            queue.pushMessage(message);
//            }
        }
        else
            std::cout << "??    " << filenameStr << '\n';
    }
}

int main(int argc, char* argv[]) {
    std::ofstream configFile;
    configFile.open(CONFIG_PATH, std::fstream::app );

    // If file does not exist, Create new file
    if (!configFile ) {
        configFile.open(CONFIG_PATH,  std::fstream::app);
        configFile <<"\n";
    }
    configFile.close();

    std::string address;
    std::string port;
    std::string username;
    std::string folder;
    std::string clientId;

    try {
        boost::program_options::options_description generic("Generic options");
        generic.add_options()
                ("version,v", "print version string")
                ("help,h", "produce help message");

        boost::program_options::options_description config("Allowed options");
        config.add_options()
                ("username,u",
                 boost::program_options::value<std::string>(&username),
                 "set your username")
                ("server-ip-address,i",
                 boost::program_options::value<std::string>(&address)->default_value("localhost"),
                 "set ip address of the remote backup server")
                ("server-port,p",
                 boost::program_options::value<std::string>(&port)->default_value("5200"),
                 "set port of the remote server")
                ("clear-config", "clear configuration previously saved");

        boost::program_options::options_description hidden("Hidden options");
        hidden.add_options()
                ("input-dir,d",
                 boost::program_options::value<std::string>(&folder),
                 "set the folder to backup")
                ("client-id",
                 boost::program_options::value<std::string>(&clientId),
                 "USE ONLY IF YOU KNOW WHAT ARE YOU DOING");

        boost::program_options::options_description cmdline_options;
        cmdline_options.add(generic).add(config).add(hidden);

        boost::program_options::options_description config_file_options;
        config_file_options.add(config).add(hidden);

        boost::program_options::options_description visible;
        visible.add(generic).add(config);

        boost::program_options::positional_options_description positionalOptions;
        positionalOptions.add("input-dir", -1);

        boost::program_options::variables_map vm;

        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(cmdline_options).positional(positionalOptions).run(), vm);

        std::fstream ifs("remote_client.cfg");
        boost::program_options::store(parse_config_file(ifs, config_file_options), vm);

        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] input-dir\n";
            std::cout << visible << "\n";
            return 0;
        }
        if (vm.count("version")) {
            std::cout << "Remote Backup Service, version " << VERSION << std::endl;
            std::cout << "Made by Daniele Leto and Daniele Palumbo" << std::endl;
            std::cout << "Programmazione di sistema, 2019/2020 " << std::endl;
            return 0;
        }
        if (!vm.count("username")) {
            std::cout << "Please add your username with -u [username]" << std::endl;
            return 1;
        }
        if (!vm.count("input-dir")) {
            std::cout << "Please add that path to the folder to backup, use -h for help" << std::endl;
            return 1;
        }
        if (!vm.count("client-id")) {
            std::cout << "Generating a random clientID...";
            clientId = randomString(64);
            std::cout << " -> your clientID is " << clientId << std::endl;
        }

        std::cout << "Welcome back user " << username << ", your clientID is " << clientId << std::endl;
        configFile.open(CONFIG_PATH, std::ofstream::out | std::ofstream::trunc);
        if (!vm.count("clear-config")) {
            std::vector<std::string> keys{"username", "server-ip-address", "server-port", "input-dir", "client-id"};
            for (const auto& key : keys) {
                std::string insert;
                if(key=="client-id" && !vm.count("client-id")) {
                    insert.append(key).append("=").append(clientId).append("\n");
                } else {
                    insert.append(key).append("=").append(vm[key].as<std::string>()).append("\n");
                }
                configFile << insert;
            }
        }

        globalClientId = clientId;

        std::string character = std::string(folder);
        character = folder.back();
        std::cout << character << std::endl;
        if (character != "/")
            folder.append("/");
        folderToWatch = folder;

        configFile.close();
    } catch (std::exception &e) {
        std::cout << "Exception during configuration: " << e.what() << std::endl;
        return 1;
    }

    std::string path_to_watch = folderToWatch;
    UploadQueue uploadQueue(100);
    bool shutdown = false;

    try {
        Message loginMessage(MessageCommand::LOGIN_REQUEST, globalClientId);
        uploadQueue.pushMessage(loginMessage);
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



