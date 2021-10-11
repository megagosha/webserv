//
// Created by George Tevosov on 10.10.2021.
//

#ifndef WEBSERV_CGIHANDLER_HPP
#define WEBSERV_CGIHANDLER_HPP
#define WRITE_END 1
#define READ_END 0

#include <map>
#include <string>
#include "HttpRequest.hpp"
#include "MimeType.hpp"
#include "VirtualServer.hpp"
#include "HttpResponse.hpp"

class VirtualServer;

class CgiHandler {
    std::map<std::string, std::string> _env;
    const HttpResponse *response;

    CgiHandler();

public:
    void CgiHandler(const HttpRequest &request, const HttpResponse &response);

    ~CgiHandler();
};

#endif //WEBSERV_CGIHANDLER_HPP
