//
// Created by George Tevosov on 10.10.2021.
//

#include "CgiHandler.hpp"

CgiHandler::CgiHandler(HttpRequest const &request, const HttpResponse &response) {
    //@todo support http authentication schemes with cgi requests
    _env["AUTH_TYPE"] = "";
    if (!request.getBody().empty()) {
        _env["CONTENT_LENGTH"] = std::to_string(request.getBody().size());
        std::map<std::string, std::string>::const_iterator it = request.getHeaderFields().find("Content-Type");
        if (it != request.getHeaderFields().end())
            _env["CONTENT_TYPE"] = it->second;
    }

    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    std::string path_info = response.getAbsolutePath().substr(response.getAbsolutePath().find_last_of('/') + 1);
    _env["PATH_INFO"] = path_info;
    if (path_info.empty())
        _env["PATH_TRANSLATED"] = "";
    else
        _env["PATH_TRANSLATED"] = response.getAbsolutePath();
    _env["QUERY_STRING"] = request.getQueryString();
    _env["REMOTE_ADDR"] = request.getClientIp();
    _env["REMOTE_HOST"] = "";
    _env["REMOTE_IDENT"] = "";
    _env["REMOTE_USER"] = "";
    _env["REQUEST_METHOD"] = request.getMethod();
    _env["SCRIPT_NAME"] = request.getRequestUri();
    std::map<std::string, std::string>::const_iterator it = request.getHeaderFields().find("Host");
    if (it == request.getHeaderFields().end())
        _env["SERVER_NAME"] = "";
    else
        _env["SERVER_NAME"] = it->second;
    _env["SERVER_PORT"] = std::to_string(response.getServ()->getPort());
    _env["SERVER_PROTOCOL"] = "HTTP/1.1";
    _env["SERVER_SOFTWARE"] = "Webserv/v0.1";
}

char **CgiHandler::GetCharArray()
{
    char **res = new char*[_env.size()];

    int i = 0;
    for (std::map<std::string, std::string>::iterator it = _env.begin(); it != _env.end(); ++it)
    {
        std::string tmp = it->first + "=" + it->second;
        res[i] = new char[tmp.size()];
        strcpy(res[i], it->second.data());
        ++i;
    }
    return (res);
}

CgiHandler::runCgi()
{
    pid_t pid;
    int fd[2];
    char  *envp[_env.size()];

    int i = 0;

    pipe(fd);
    pid=fork();

    if(pid == 0)
    {
        dup2(fd[READ_END], STDIN_FILENO);
        close(fd[WRITE_END]);
        close(fd[READ_END]);
        //set current working directory to directory containing script (FILE OR PHP FPM)? 
        int res =  execve(response->getLoc()->getCgiPass().data(), NULL,
                          reinterpret_cast<char *const *>(envp));

        fprintf(stderr, "Failed to execute '%s'\n", scmd);
        exit(1);
    }
    else
    {
        int status;
        close(fd[READ_END]);
        close(fd[WRITE_END]);
        waitpid(pid, &status, 0);
    }
}
CgiHandler::~CgiHandler() {

}

CgiHandler::CgiHandler() {

}
