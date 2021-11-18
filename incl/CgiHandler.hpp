//
// Created by George Tevosov on 16.11.2021.
//

#ifndef WEBSERV_CGIHANDLER_HPP
#define WEBSERV_CGIHANDLER_HPP

#include <map>
#include <iostream>
#include<fstream>

#include <fstream>
#include <vector>
#include <string>
#include <cstddef>
#include <Socket.hpp>
#include "Session.hpp"
#include "FileStats.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include "HttpRequest.hpp"

class CgiHandler {
public:

    ~CgiHandler();

    CgiHandler(CgiHandler const &rhs);

    CgiHandler &operator=(CgiHandler const &rhs);

    CgiHandler(pid_t pid, HttpRequest *req);

    bool prepareCgiEnv(HttpRequest *request, const std::string &absolute_path, const std::string& serv_port, const std::string &cgi_exec);

    pid_t getCgiPid() const;

    void setCgiPid(pid_t cgiPid);

    HttpRequest *getReq() const;

    void setReq(HttpRequest *req);

    size_t getPos() const;

    void setPos(size_t pos);

    bool isHeadersParsed() const;

    void setHeadersParsed(bool);


    class CgiException : public std::exception {
        const std::string m_msg;
    public:
        CgiException(const std::string &msg);

        ~CgiException() throw();

        const char *what() const throw();
    };

    CgiHandler();

    void setRequestPipe(int requestPipe);

    void setResponsePipe(int responsePipe);

    const std::string &getCgiPath() const;

    void setCgiPath(const std::string &cgiPath);

    const std::map<std::string, std::string> &getEnv() const;

    char **getEnv(void);

    int getRequestPipe() const;

    int getResponsePipe() const;

private:
    pid_t _cgi_pid;
    int   _request_pipe;
    int                                _response_pipe;
    HttpRequest                        *_req;
    size_t                             _pos;
    std::map<std::string, std::string> _env;
    std::string                        _cgi_path;
    bool                               _headers_parsed;
};


#endif //WEBSERV_CGIHANDLER_HPP
