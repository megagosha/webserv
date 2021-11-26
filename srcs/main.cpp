#include "Server.hpp"

int main(int argc, char **argv) {
    if (argc < 2)
        std::cout << "Specify path to config file" << std::endl;
    std::string path(argv[1]);
    Server x(path);
    x.loop();
}