#include <iostream>
#include "Server.h"

int main() {
    try{

        //watch the port
        short port = 2000;

        boost::asio::io_service ioService;
        Server server(ioService, port);
        ioService.run();
    }

    catch(std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
