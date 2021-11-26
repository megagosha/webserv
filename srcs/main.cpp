#include "Server.hpp"

int main(int argc, char **argv) {
	std::string path = "webserv_config_example";
	if (argc > 1)
		path = argv[1];
	Server x(path);
    x.loop();
}
