#include <iostream>
#include <boost/asio/io_service.hpp>
#include "Server.h"
#include "Logger.h"


int main() {
    try{

        //watch the port
        short port = 2000;

        // default working directory
        std::string rootDirectory = "./files/";

        boost::asio::io_service ioService;
        Server server(ioService, port, rootDirectory);
        ioService.run();
    }

    catch(std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
