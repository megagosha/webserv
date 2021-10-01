#include <string>
#include <iostream>
//#include "Server.hpp"

//
//int main(void) {
//	Server("/Users/edebi/Desktop/webserv/test3.txt");
//}
#include <list>




int main(void) {
    std::pair<std::string, bool> res;
    res = normalize_path("/sdfasdf/asdfasdffsd/");
    std::cout << "RESULT: " << res.second << std::endl;
}