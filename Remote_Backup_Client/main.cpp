#include <iostream>
#include <boost/program_options.hpp>
#include <random>
#include <functional>
#include <csignal>
#include "FileWatcher.h"
#include "Client.h"
#include "UploadQueue.h"
#include "Message.h"
#include "FileToUpload.h"
#include "BackupClient.h"

#define VERSION       "0.1"
#define CONFIG_PATH   "remote_client.cfg"
#define CLIENTID_PATH "client_identifier.cfg"

std::string globalClientId;
std::string folderToWatch;
bool shutdownComplete = false;
bool clientResponse;
BackupClient backupClient;
int ctrlc_times = 0;

void initializeConfigFiles(std::fstream &configFile, std::fstream &clientIdFile);

void timer_callback(const boost::system::error_code& errorCode) {
    if(backupClient.get_uploadQueue()->queueSize() != 0) {
        std::cout << "\tTIMER EXPIRED -> force close" << std::endl;
        std::cout << "-> Shutdown complete, byebye!" << std::endl;
        exit(2);
    }
}

void signal_callback_handler(int signum) {
    ctrlc_times++;
    if(signum == 2 && ctrlc_times==1) {
        std::cout << "\n\n4. SHUTDOWN in progress..." << std::endl;
        if(backupClient.get_fileWatcher() != nullptr)
            backupClient.get_fileWatcher()->stop();
        std::cout << "\tFilewatcher stopped" << std::endl;

        boost::asio::io_service ios;
        boost::asio::deadline_timer timer(ios, boost::posix_time::seconds(10));
        timer.async_wait(&timer_callback);

        if(backupClient.get_uploadQueue() != nullptr && backupClient.get_uploadQueue()->queueSize() != 0) {
            std::cout << "\tStarted timeout for force shutdown in 10 seconds" << std::endl;
        }
        std::thread teq([&timer](){
           backupClient.get_uploadQueue()->readyToClose();
           if(backupClient.get_currentClient()== nullptr)
               timer.cancel();
        });
        teq.detach();
        ios.run();
        std::cout << "-> Shutdown complete, byebye!" << std::endl;
        exit(signum);
    } else {
        exit(signum);
    }
}

std::string randomString(size_t length ) {
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
    Client client(address, port, message);
    backupClient.set_currentClient(client);
    client.start();
    if (message.getCommand() == MessageCommand::INFO_REQUEST) {
        clientResponse = client.getResponse();
    }
    backupClient.delete_currentClient();
}

void run_file_watcher(const std::string & path_to_watch, UploadQueue& queue) {
    try {
        // Create a FileWatcher instance that will check the current folder for changes every 1 second
        FileWatcher fileWatcher{path_to_watch, std::chrono::milliseconds(1000)};
        backupClient.set_fileWatcher(fileWatcher);

        // Start monitoring a folder for changes and (in case of changes)
        // run a user provided lambda function
        fileWatcher.start([&](const std::string & path_to_watch, FileStatus status) -> void {

            // Process only regular files, all other file types are ignored
            std::filesystem::path filePath(path_to_watch);
            if (!std::filesystem::is_regular_file(filePath) && status != FileStatus::erased) {
                return;
            }

            switch (status) {
                case FileStatus::created: {
                    FileToUpload fileToUpload(folderToWatch, filePath);
                    Message message(MessageCommand::CREATE, fileToUpload, globalClientId);
                    queue.pushMessage(message);
                    break;
                }
                case FileStatus::modified: {
                    FileToUpload fileToUpload(folderToWatch, filePath);
                    Message message(MessageCommand::CREATE, fileToUpload, globalClientId);
                    queue.pushMessage(message);
                    break;
                }
                case FileStatus::erased: {
                    FileToUpload fileToUpload(folderToWatch, filePath);
                    Message message(MessageCommand::REMOVE, fileToUpload, globalClientId);
                    queue.pushMessage(message);
                    break;
                }
                default:
                    std::cout << "\nERROR! Unknown file status.\n" << std::endl;
            }
        });
    }
    catch (std::exception &e) {
        std::cout << "Exception!" << std::endl;
        std::cout << e.what() << std::endl;
    }
}

void scan_directory(const std::string& path_to_watch, UploadQueue& queue, const std::string& address, const std::string& port) {
    for(auto itEntry = std::filesystem::recursive_directory_iterator(path_to_watch);
        itEntry != std::filesystem::recursive_directory_iterator();
        ++itEntry ) {
        const auto filenameStr = itEntry->path().filename().string();
        if (itEntry->is_regular_file()) {
            FileToUpload fileToUpload(folderToWatch, itEntry->path());
            std::string hash = fileToUpload.fileHash();

            Message message(MessageCommand::INFO_REQUEST, fileToUpload, globalClientId);

            createClientSend(message, address, port);
            if(clientResponse == 0) {
                message.setCommand(MessageCommand::CREATE);
                std::cout << "\tFILE: " << filenameStr << " not present on server -> enqueue to send to server" << std::endl;
                queue.pushMessage(message);
            } else {
                std::cout << "\tFILE: " << filenameStr << " already present on server -> skipping" << std::endl;
            }
        } else if(itEntry->is_directory()){
            std::cout << "\tDIR: " << filenameStr << std::endl;
        } else
            std::cout << "??    " << filenameStr << std::endl;
    }
}

int main(int argc, char* argv[]) {

    std::cout << "============= REMOTE BACKUP CLIENT =============" << "\n\n";

    std::fstream configFile(CONFIG_PATH);
    std::fstream clientIdFile(CLIENTID_PATH);
    std::string address;
    std::string port;
    std::string username;
    std::string folder;
    std::string clientId;

    std::cout << "1. Service configuration phase..." << std::endl;
    initializeConfigFiles(configFile, clientIdFile);

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
                 "set the folder to backup");

        boost::program_options::options_description clientIdOption("Hidden Client ID");
        clientIdOption.add_options()
                ("client-id",
                 boost::program_options::value<std::string>(&clientId),
                 "USE ONLY IF YOU KNOW WHAT ARE YOU DOING");

        boost::program_options::options_description cmdline_options;
        cmdline_options.add(generic).add(config).add(hidden).add(clientIdOption);

        boost::program_options::options_description config_file_options;
        config_file_options.add(config).add(hidden);

        boost::program_options::options_description visible;
        visible.add(generic).add(config);

        boost::program_options::positional_options_description positionalOptions;
        positionalOptions.add("input-dir", -1);

        boost::program_options::variables_map vm;

        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(cmdline_options).positional(positionalOptions).run(), vm);

        std::fstream ifs(CONFIG_PATH);
        std::fstream cfs(CLIENTID_PATH);
        boost::program_options::store(parse_config_file(ifs, config_file_options), vm);
        boost::program_options::store(parse_config_file(cfs, clientIdOption), vm);

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
        } else {
            char lastChar = folder.back();
            if (lastChar != '/')
                folder.append("/");
        }
        if (!vm.count("client-id")) {
            std::cout << "Generating a random clientID...";
            clientId = randomString(64);
            std::cout << " -> your clientID is " << clientId << std::endl;
        }

        std::cout << "-> Service configuration done! Welcome back user " << username << ", your clientID is " << clientId << "\n\n";
        configFile.open(CONFIG_PATH, std::ofstream::out | std::ofstream::trunc);
        if (!vm.count("clear-config")) {
            std::vector<std::string> keys{"username", "server-ip-address", "server-port", "input-dir"};
            for (const auto& key : keys) {
                std::string insert;
                insert.append(key).append("=").append(vm[key].as<std::string>()).append("\n");
                configFile << insert;
            }
        }
        clientIdFile.open(CLIENTID_PATH, std::ofstream::out | std::ofstream::trunc);
        std::string insert;
        insert.append("client-id").append("=").append(clientId).append("\n");
        clientIdFile << insert;

        globalClientId = clientId;
        folderToWatch = folder;

        configFile.close();
        clientIdFile.close();
    } catch (std::exception &e) {
        std::cout << "Exception during configuration: " << e.what() << std::endl;
        return 1;
    }

    UploadQueue uploadQueue(10000);
    backupClient.set_uploadQueue(uploadQueue);

    signal(SIGINT, signal_callback_handler);

    try {
        Message loginMessage(MessageCommand::LOGIN_REQUEST, globalClientId);
        uploadQueue.pushMessage(loginMessage);

        std::cout << "2. Check current directory status..." << std::endl;
        scan_directory(folderToWatch, uploadQueue, address, port);
        std::cout << "-> Client and server file system aligned\n\n";

        //TODO: control the path, if not exists -> raise exception
        std::thread tfw([&uploadQueue](){
            run_file_watcher(folderToWatch, uploadQueue);
        });

        std::thread tcq([&uploadQueue, &address, &port](){
            while(!shutdownComplete) {
                Message message = uploadQueue.popMessage();
                createClientSend(message, address, port);
            }
        });

        std::cout << "3. Ready to follow your folder evolution..." << std::endl;
        tfw.join();
        tcq.join();
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Exceptions!" << std::endl;
    }

    std::cout << "\n\n" << "============= BYE BYE =============" << "\n\n";

    return 0;
}

void initializeConfigFiles(std::fstream &configFile, std::fstream &clientIdFile) {
    configFile.open(CONFIG_PATH, std::fstream::app);
    clientIdFile.open(CLIENTID_PATH, std::fstream::app);

    // If file does not exist, Create new file
    if (!configFile ) {
        configFile <<"\n";
    }

    if (!clientIdFile ) {
        clientIdFile <<"\n";
    }

    configFile.close();
    clientIdFile.close();
}



