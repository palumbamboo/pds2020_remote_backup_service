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

#define VERSION       "1.0"
#define CONFIG_PATH   "remote_client.cfg"

std::string globalClientId;
std::string folderToWatch;
bool shutdownComplete = false;
bool serverResponse;
BackupClient backupClient;

void initializeConfigFiles(std::fstream &configFile);
void performLogin(int times, const std::string& username, const std::string& address, const std::string& port);

void timer_callback(const boost::system::error_code& errorCode) {
    if(backupClient.get_uploadQueue()->queueSize() != 0) {
        std::cout << "\tTIMER EXPIRED -> force close" << std::endl;
        std::cout << "-> Shutdown complete, byebye!" << std::endl;
        exit(2);
    }
}

void signal_callback_handler(int signum) {
    std::cout << "\n\n4. SHUTDOWN in progress..." << std::endl;
    if(backupClient.get_fileWatcher() != nullptr)
        backupClient.get_fileWatcher()->stop();
    std::cout << "\tFILEWATCHER -> stopped" << std::endl;

    if(backupClient.get_currentClient() != nullptr) {
        backupClient.delete_currentClient();
    }
    std::cout << "\tOPEN CLIENT -> closed" << std::endl;

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
}

std::string passwordHash(std::string s) {
    std::string hashResult;
    boost::uuids::detail::md5 hash;
    boost::uuids::detail::md5::digest_type digest;

    hash.process_bytes(s.data(), s.size());
    hash.get_digest(digest);

    std::string result;

    const auto charDigest = reinterpret_cast<const char *>(&digest);
    boost::algorithm::hex(charDigest, charDigest + sizeof(boost::uuids::detail::md5::digest_type),
                          std::back_inserter(result));
    hashResult = result;
    return hashResult;
}

void createClientSend(Message& message, const std::string& address, const std::string& port) {
    Client client(address, port, message);
    backupClient.set_currentClient(client);
    MessageCommand command = message.getCommand();

    client.start();

    switch (command) {
        // LOGIN_REQUEST | | username | | hashed password
        case MessageCommand::LOGIN_REQUEST: {
            serverResponse = client.getResponse();
            globalClientId = client.getClientId();
            break;
        }
            // INFO_REQUEST | | clientID | | path | | hashed file
        case MessageCommand::INFO_REQUEST: {
            serverResponse = client.getResponse();
            break;
        }
            // END_INFO_PHASE | | clientID
        case MessageCommand::END_INFO_PHASE: {
            serverResponse = client.getResponse();
            break;
        }
            // REMOVE | | clientID | | path
        case MessageCommand::REMOVE: {
            serverResponse = client.getResponse();
            break;
        }
            // CREATE | | clientID | | path | | file size
            // file data inside request body
        case MessageCommand::CREATE: {
            serverResponse = client.getResponse();
            break;
        }
        default: {
            std::cout << "\tERROR -> unknown command!" << std::endl;
            return;
        }
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
                    FileToUpload fileToUpload(folderToWatch, filePath, std::filesystem::file_size(filePath));
                    Message message(MessageCommand::CREATE, fileToUpload, globalClientId);
                    queue.pushMessage(message);
                    break;
                }
                case FileStatus::modified: {
                    FileToUpload fileToUpload(folderToWatch, filePath, std::filesystem::file_size(filePath));
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
        std::string error = "FATAL ERROR FILEWATCHER: ";
        error.append(e.what());
        throw std::runtime_error(error);
    }
}

void scan_directory(const std::string& path_to_watch, UploadQueue& queue, const std::string& address, const std::string& port, const std::string& forceAlignment) {
    for(auto itEntry = std::filesystem::recursive_directory_iterator(path_to_watch);
        itEntry != std::filesystem::recursive_directory_iterator();
        ++itEntry ) {
        const auto filenameStr = itEntry->path().filename().string();
        if (itEntry->is_regular_file()) {
            FileToUpload fileToUpload(folderToWatch, itEntry->path(), std::filesystem::file_size(itEntry->path()));
            std::string hash = fileToUpload.fileHash();

            Message message(MessageCommand::INFO_REQUEST, fileToUpload, globalClientId);

            createClientSend(message, address, port);
            if(serverResponse == 0) {
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

    Message message(MessageCommand::END_INFO_PHASE, globalClientId, false);

    if(forceAlignment == "true") {
        message.setForceAlignment(true);
    }

    createClientSend(message, address, port);
    if(serverResponse != 1) {
        std::cout << "\tERROR with client and server folders synchronization" << std::endl;
        exit(1);
    } else {
        if(forceAlignment == "true") {
            std::cout << "-> Client and server file system aligned (filesystem FORCED to match that of the client)\n\n";
        } else {
            std::cout << "-> Client and server file system aligned\n\n";
        }
    }
}

int main(int argc, char* argv[]) {

    std::cout << "\n============= REMOTE BACKUP CLIENT =============" << "\n\n";

    std::fstream configFile(CONFIG_PATH);
    std::string address;
    std::string port;
    std::string username;
    std::string folder;
    std::string forceAlignFs;

    std::cout << "1. Service configuration phase..." << std::endl;
    initializeConfigFiles(configFile);

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
                ("force-align-fs,f",
                 boost::program_options::value<std::string>(&forceAlignFs)->default_value("false"),
                 "set true to enforce server fs alignment with client")
                ("clear-config", "clear configuration previously saved");

        boost::program_options::options_description hidden("Hidden options");
        hidden.add_options()
                ("input-dir,d",
                 boost::program_options::value<std::string>(&folder),
                 "set the folder to backup");

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

        std::fstream ifs(CONFIG_PATH);
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
            std::cout << "-> ERROR: Please add your username with -u [username]" << std::endl;
            return 1;
        }
        if (!vm.count("input-dir")) {
            std::cout << "-> ERROR: Please add that path to the folder to backup, use -h for help" << std::endl;
            return 1;
        } else {
            char lastChar = folder.back();
            if (lastChar != '/')
                folder.append("/");
            std::filesystem::path path(vm["input-dir"].as<std::string>());
            if(!std::filesystem::exists(path)) {
                std::cout << "-> ERROR: configured path does not exist!" << std::endl;
                exit(1);
            }
        }

        configFile.open(CONFIG_PATH, std::ofstream::out | std::ofstream::trunc);
        if (!vm.count("clear-config")) {
            std::vector<std::string> keys{"username", "server-ip-address", "server-port", "input-dir", "force-align-fs"};
            for (const auto& key : keys) {
                std::string insert;
                insert.append(key).append("=").append(vm[key].as<std::string>()).append("\n");
                configFile << insert;
            }
        } else {
            std::cout << "-> Service configuration cleared! See how to reconfigure running with -h\n\n";
            exit(1);
        }

        folderToWatch = folder;

        configFile.close();
    } catch (std::exception &e) {
        std::cout << "Exception during configuration: " << e.what() << std::endl;
        return 1;
    }

    UploadQueue uploadQueue(10000);
    backupClient.set_uploadQueue(uploadQueue);

    signal(SIGINT, signal_callback_handler);

    try {
        int timesLogin = 0;
        while(timesLogin < 3 && !serverResponse) {
            performLogin(timesLogin, username, address, port);
            timesLogin++;
        }

        if (globalClientId == "0") {
            std::cout << "-> Program execution aborted! You insert the wrong password 3 times!" << "\n\n";
            return 1;
        }

        std::cout << "-> Service configuration done! Welcome back user " << username << ", your clientID is " << globalClientId << "\n\n";
        std::cout << "2. Check current directory status..." << std::endl;
        scan_directory(folderToWatch, uploadQueue, address, port, forceAlignFs);

        std::thread tfw([&uploadQueue](){
            run_file_watcher(folderToWatch, uploadQueue);
        });

        std::thread tcq([&uploadQueue, &address, &port](){
            while(!shutdownComplete) {
                Message message = uploadQueue.popMessage();
                createClientSend(message, address, port);
                if(serverResponse != 1) {
                    if(message.getCommand() == MessageCommand::CREATE || message.getCommand() == MessageCommand::REMOVE) {
                        std::cout << " -> ERROR server side: retry command" << std::endl;
                        uploadQueue.pushMessage(message);
                    }
                } else {
                    std::cout << "\t\t -> DONE" << std::endl;
                }
            }
        });

        std::cout << "3. Ready to follow your folder evolution..." << std::endl;
        tfw.join();
        tcq.join();
    }
    catch (std::exception& e) {
        std::cerr << "\n\nSERVICE STOPPED, due to " << e.what() << std::endl;
        std::cerr << "Please retry later!" << std::endl;
    }

    std::cout << "\n\n" << "============= BYE BYE =============" << "\n\n";

    return 0;
}

void performLogin(int times, const std::string& username, const std::string& address, const std::string& port) {
    std::string password;
    std::string hashedPassword;

    if(times == 0) {
        std::cout << "\tWelcome user " << username << ", please insert your password here: ";
    } else {
        std::cout << "\tWrong password inserted, " << username << ", please retry: ";
    }

    // Hide terminal echo to insert user password
    termios oldTerminal;
    tcgetattr(STDIN_FILENO, &oldTerminal);
    termios newTerminal = oldTerminal;
    newTerminal.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerminal);
    getline(std::cin, password);
    // Restore previous terminal
    tcsetattr(STDIN_FILENO, TCSANOW, &oldTerminal);
    std::cout << std::endl;

    hashedPassword = passwordHash(password);
    Message loginMessage(MessageCommand::LOGIN_REQUEST, username, hashedPassword);
    createClientSend(loginMessage, address, port);
}

void initializeConfigFiles(std::fstream &configFile) {
    configFile.open(CONFIG_PATH, std::fstream::app);

    // If file does not exist, Create new file
    if (!configFile ) {
        configFile <<"\n";
    }

    configFile.close();
}



