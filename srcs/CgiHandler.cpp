//
// Created by George Tevosov on 16.11.2021.
//

#include "CgiHandler.hpp"

pid_t CgiHandler::getCgiPid() const {
    return _cgi_pid;
}

void CgiHandler::setCgiPid(pid_t cgiPid) {
    _cgi_pid = cgiPid;
}

HttpRequest *CgiHandler::getReq() const {
    return _req;
}

void CgiHandler::setReq(HttpRequest *req) {
    _req = req;
}

size_t CgiHandler::getPos() const {
    return _pos;
}

void CgiHandler::setPos(size_t pos) {
    _pos = pos;
}

bool CgiHandler::isHeadersParsed() const {
    return _headers_parsed;
}

void CgiHandler::setHeadersParsed(bool status) {
    _headers_parsed = status;
}


CgiHandler::~CgiHandler() {

}

CgiHandler::CgiHandler(const CgiHandler &rhs) : _cgi_pid(rhs._cgi_pid), _req(rhs._req), _pos(rhs._pos),
                                                _headers_parsed(rhs._headers_parsed) {

}

CgiHandler &CgiHandler::operator=(const CgiHandler &rhs) {
    if (this == &rhs)
        return (*this);
    _headers_parsed = rhs._headers_parsed;
    _cgi_pid        = rhs._cgi_pid;
    _pos            = rhs._pos;
    _req            = rhs._req;
    return (*this);
}

CgiHandler::CgiHandler(pid_t pid, HttpRequest *req) : _cgi_pid(pid), _req(req), _headers_parsed(false) {

}

bool CgiHandler::prepareCgiEnv(HttpRequest *request, const std::string &absolute_path, const std::string &serv_port,
                               const std::string &cgi_exec) {
    _env["AUTH_TYPE"]      = "";
//	if (!request.getBody().empty())
//	{
    _env["CONTENT_LENGTH"] = std::to_string(request->getBody().size());
    std::map<std::string, std::string>::iterator it = request->getHeaderFields().find("Content-Type");
    if (it != request->getHeaderFields().end())
        _env["CONTENT_TYPE"] = it->second;
    else
        _env["CONTENT_TYPE"]  = "";
//	}
    _env["GATEWAY_INTERFACE"] = std::string("CGI/1.1");
//    std::string path_info = absolute_path.substr(absolute_path.find_last_of('/') + 1);
    if (!cgi_exec.empty())
        _env["SCRIPT_NAME"]   = "/Users/megagosha/Downloads/cgi_tester";//cgi_exec;//"/index.php";//request.getRequestUri();
    _env["SCRIPT_FILENAME"]   = absolute_path;

    _env["PATH_TRANSLATED"] = request->getRequestUri(); //@todo add additional string manipulation as in rfc
    _env["PATH_INFO"]       = request->getRequestUri();
    _env["QUERY_STRING"]    = request->getQueryString();
    _env["REQUEST_URI"]     = request->getRequestUri();
    _env["REMOTE_ADDR"]     = request->getClientIp();
    _env["REMOTE_HOST"]     = "0.0.0.0";
    _env["REMOTE_IDENT"]    = "";
    _env["REMOTE_USER"]     = "";
    _env["REQUEST_METHOD"]  = request->getMethod();
    it = request->getHeaderFields().find("Host");
    if (it == request->getHeaderFields().end())
        _env["SERVER_NAME"] = "0"; //@todo check logic Maybe get server name from VirtualServer config
    else
        _env["SERVER_NAME"] = it->second;
    _env["SERVER_PORT"]     = serv_port; // + '\0';
    _env["SERVER_PROTOCOL"] = "HTTP/1.1";
    _env["SERVER_SOFTWARE"] = "topserv_v0.1";
    std::map<std::string, std::string>           tmp_map;
    std::map<std::string, std::string>::iterator tmp_map_iter;
    for (it = request->getHeaderFields().begin(); it != request->getHeaderFields().end();) {
        std::string tmp = it->first;
        std::replace(tmp.begin(), tmp.end(), '-', '_');
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
        tmp_map["HTTP_" + tmp] = it->second;
        tmp_map_iter = it;
        ++tmp_map_iter;
        request->getHeaderFields().erase(it);
        it = tmp_map_iter;
    }
    _env.insert(tmp_map.begin(), tmp_map.end());
    for (it = _env.begin(); it != _env.end(); ++it) {
        std::cout << it->first << ": " << it->second << std::endl;
    }

    return (true);
}

const std::map<std::string, std::string> &CgiHandler::getEnv() const {
    return _env;
}

const std::string &CgiHandler::getCgiPath() const {
    return _cgi_path;
}

void CgiHandler::setCgiPath(const std::string &cgiPath) {
    _cgi_path = cgiPath;
}

char **CgiHandler::getEnv(void) {
    char **res = new char *[_env.size() + 1];
    int  i     = 0;

    for (std::map<std::string, std::string>::const_iterator it = _env.begin(); it != _env.end(); ++it) {
        if (it->second.empty())
            continue;
        std::string tmp = it->first + "=" + it->second;
        res[i] = new char[tmp.size() + 1];
        strcpy(res[i], tmp.data());
        ++i;
    }
    res[i]                                                     = nullptr;
    return (res);
}

void CgiHandler::setRequestPipe(int requestPipe) {
    _request_pipe = requestPipe;
}

void CgiHandler::setResponsePipe(int responsePipe) {
    _response_pipe = responsePipe;
}

int CgiHandler::getRequestPipe() const {
    return _request_pipe;
}

int CgiHandler::getResponsePipe() const {
    return _response_pipe;
}

CgiHandler::CgiHandler() :
        _cgi_pid(0),
        _request_pipe(-1),
        _response_pipe(-1),
        _req(nullptr),
        _pos(0),
        _headers_parsed(false) {
}


bool CgiHandler::cgiEnd() {
    int res;
    res = waitpid(_cgi_pid, &_exit_status, WNOHANG);
    if (res == -1)
        throw CgiException("waitpid returned -1");
    else if (res == 0)
        kill(_cgi_pid, SIGKILL);
    else if (WIFEXITED(_exit_status) && WEXITSTATUS(_exit_status) == 0)
        return (true);
    return (false);
}

CgiHandler::CgiException::CgiException(const std::string &msg) {
    std::cout << msg << std::endl;
}

CgiHandler::CgiException::~CgiException() throw() {

}

const char *CgiHandler::CgiException::what() const throw() {
    return exception::what();
}