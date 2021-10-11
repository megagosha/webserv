//
// Created by George Tevosov on 10.10.2021.
//

#ifndef WEBSERV_CGIHANDLER_HPP
#define WEBSERV_CGIHANDLER_HPP
#include <map>
#include <string>
#include "HttpRequest.hpp"
#include "MimeType.hpp"
class CgiHandler {
private:
	std::map<std::string, std::string> _env;
	void setEnv(HttpRequest const &request);
CgiHandler();
~CgiHandler();
};
#endif //WEBSERV_CGIHANDLER_HPP
