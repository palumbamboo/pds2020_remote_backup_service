#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <map>
#include <vector>

#include "Server.h"
#include "UserMap.h"

#define VERSION     "1.0"
#define CONFIG_PATH "remote_server.cfg"

std::shared_ptr<boost::asio::io_service> ioServicePointer;

void load_users_password();
void initializeConfigFiles(std::fstream &configFile, std::fstream &passFile);

void signal_callback_handler(int signum) {
    std::cout << "\n\n3. SHUTDOWN in progress..." << std::endl;

    ioServicePointer->stop();
    std::cout << "\tSERVER -> closed" << std::endl;

    std::cout << "-> Shutdown complete, byebye!" << std::endl;
    exit(signum);
}

int main(int argc, char* argv[]) {
    std::cout << "============= REMOTE BACKUP SERVER =============" << "\n\n";

    std::fstream configFile(CONFIG_PATH);
    std::fstream passFile(PASS_PATH);
    std::string port;

    std::cout << "1. Server configuration phase..." << std::endl;

    try {
        initializeConfigFiles(configFile, passFile);
        load_users_password();

        boost::program_options::options_description generic("Generic options");
        generic.add_options()
                ("version,v", "print version string")
                ("help,h", "produce help message");

        boost::program_options::options_description config("Allowed options");
        config.add_options()
                ("server-port,p",
                 boost::program_options::value<std::string>(&port)->default_value("5200"),
                 "set port of the remote server")
                ("clear-config", "clear configuration previously saved");

        boost::program_options::options_description cmdline_options;
        cmdline_options.add(generic).add(config);

        boost::program_options::options_description config_file_options;
        config_file_options.add(config);

        boost::program_options::options_description visible;
        visible.add(generic).add(config);

        boost::program_options::variables_map vm;

        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(cmdline_options).run(), vm);

        std::fstream ifs(CONFIG_PATH);
        boost::program_options::store(parse_config_file(ifs, config_file_options), vm);

        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << visible << "\n";
            return 0;
        }
        if (vm.count("version")) {
            std::cout << "Remote Backup Service, version " << VERSION << std::endl;
            std::cout << "Made by Daniele Leto and Daniele Palumbo" << std::endl;
            std::cout << "Programmazione di sistema, 2019/2020 " << std::endl;
            return 0;
        }
        if (vm.count("port")) {
            int m_port = vm["port"].as<int>();
            if(m_port < 0 || m_port > 65535) {
                std::cout << "-> ERROR: invalid port #" << m_port << std::endl;
                return 1;
            }
        }

        configFile.open(CONFIG_PATH, std::ofstream::out | std::ofstream::trunc);
        if (!vm.count("clear-config")) {
            std::vector<std::string> keys{"server-port"};
            for (const auto& key : keys) {
                std::string insert;
                insert.append(key).append("=").append(vm[key].as<std::string>()).append("\n");
                configFile << insert;
            }
        } else {
            std::cout << "-> Service configuration cleared! See how to reconfigure running with -h\n\n";
            exit(1);
        }

        std::string insert;

        configFile.close();

    } catch (std::exception &e) {
        std::cout << "-> ERROR: exception during configuration due to " << e.what() << std::endl;
        exit(1);
    }

    signal(SIGINT, signal_callback_handler);

    try {
        ioServicePointer = std::make_shared<boost::asio::io_service>();
        Server server(*ioServicePointer, port);

        std::cout << "2. Server in listen mode..." << std::endl;

        ioServicePointer->run();
    }

    catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

void load_users_password() {
    std::ifstream passFile(PASS_PATH);
    std::string fl;
    getline(passFile, fl);
    if(!fl.empty()) {
        passFile.close();
        loadUserLoginMap();
    } else {
        passFile.close();
    }
}

void initializeConfigFiles(std::fstream &configFile, std::fstream &passFile) {
    configFile.open(CONFIG_PATH, std::fstream::app);
    passFile.open(PASS_PATH, std::fstream::app);

    // If file does not exist, Create new file
    if (!configFile) {
        configFile <<"\n";
    }

    if (!passFile) {
        passFile <<"\n";
    }

    configFile.close();
    passFile.close();
}
