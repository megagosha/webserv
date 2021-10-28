#include "Server.hpp"
#include "unistd.h"
#include "limits.h"  //Для PATH_MAX

int main (void)
{
    //GetCurrentDirectory()
    char PathName[PATH_MAX];
    getwd(PathName);
    std::string c_path = (std::string)PathName + "/../webserv_config_example";
    Server x(c_path);

	//Server x("/Users/mac/CLionProjects/webserv/webserv_config_example");
}
