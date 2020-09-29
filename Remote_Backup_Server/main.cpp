#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>

#include "Server.h"
#include "UserMap.h"

#define VERSION         "0.1"
#define CONFIG_PATH     "remote_server.cfg"

nlohmann::json jsonMap;

void load_users_password() {
    std::ifstream passFile(PASS_PATH);
    std::string fl;
    getline(passFile, fl);
    if(!fl.empty()) {
        passFile.close();
        std::ifstream passFile2(PASS_PATH);
        passFile2 >> jsonMap;
        userMap = jsonMap.get<std::map<std::string, std::vector<std::string>>>();
        passFile2.close();
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

int main(int argc, char* argv[]) {


//    std::vector<std::string> ciao{"cavallo", "bello"};
//    userMap.insert(std::pair<std::string, std::vector<std::string>>("gianni", ciao));


    try{

        std::cout << "============= REMOTE BACKUP SERVER =============" << "\n\n";

        std::fstream configFile(CONFIG_PATH);
        std::fstream passFile(PASS_PATH);
        std::string port;

        initializeConfigFiles(configFile, passFile);
        load_users_password();

        try {
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

            configFile.open(CONFIG_PATH, std::ofstream::out | std::ofstream::trunc);
            if (!vm.count("clear-config")) {
                std::vector<std::string> keys{"server-port"};
                for (const auto& key : keys) {
                    std::string insert;
                    insert.append(key).append("=").append(vm[key].as<std::string>()).append("\n");
                    configFile << insert;
                }
            }

            std::string insert;

            configFile.close();

        } catch (std::exception &e) {
            std::cout << "Exception during configuration: " << e.what() << std::endl;
            return 1;
        }

        boost::asio::io_service ioService;
        Server server(ioService, port);
        ioService.run();
    }

    catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
