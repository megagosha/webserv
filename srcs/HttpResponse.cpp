//
// Created by Elayne Debi on 9/9/21.
//

#include "HttpResponse.hpp"

void HttpResponse::setResponseString(std::string pr, std::string s_c, std::string s_r) {
    _proto = pr;
    _status_code = s_c;
    _status_reason = s_r;
    _response_string = _proto + " " + _status_code + " " + _status_reason + "\r\n";
}

short HttpResponse::writeFileToBuffer(std::string &file_path) {
    long long int length;
    std::ifstream file(file_path, std::ifstream::in | std::ifstream::binary);
    if (file) {
        file.seekg(0, file.end);
        length = file.tellg();
        if (length <= 0) {
            _body_size = 0;
            return (500);
        }
        file.seekg(0, file.beg);
        _body_size = (std::size_t) length;
        _body.reserve(_body.size() + length);
        _body.insert(_body.end(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        if (file.peek() != EOF) {
            _body_size = 0;
            return (500);
        }
        file.close();
        insertHeader("Content-Type", MimeType::getType(file_path));//@todo determine content type
        return (200);
    } else
        return (404);
}

void HttpResponse::insertHeader(std::string name, std::string value) {
    _header.insert(std::make_pair(name, value));
}

void HttpResponse::setTimeHeader(void) {
    time_t now = time(nullptr);
    char buf[100];

    std::strftime(buf, 100, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&now));
    insertHeader("Date", buf);
    insertHeader("Server", "mg_webserv/0.01");
}

std::string HttpResponse::getReasonPhrase(short error_code) {
    std::string reason_phrase;

    switch (error_code) {
        case 200 : {
            reason_phrase = "OK";
            break;
        }
        case 404 : {
            reason_phrase = "Not Found";
            break;
        }
        case 400 : {
            reason_phrase = "Bad Request";
            break;
        }
        case 403 : {
            reason_phrase = "Forbidden";
            break;
        }
        case 405 : {
            reason_phrase = "Method not allowed";
            break;
        }
        case 410 : {
            reason_phrase = "Gone";
            break;
        }
        case 413 : {
            reason_phrase = "Payload is too large";
            break;
        }
        case 500 : {
            reason_phrase = "Internal error occured";
            break;
        }
        default:
            reason_phrase = "Error";
    }
    return (reason_phrase);
}

std::string HttpResponse::getErrorHtml(std::string &error, std::string &reason) {
    std::string res;

    res += "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"/><title>" + error +
           "</title></head><body><h1>" + error + " " + reason + "</h1></body></html>\n";
    return (res);
}


HttpResponse::HttpResponse() {
}

void HttpResponse::setError(short int code, const VirtualServer &server) {
    std::string error_code;
    std::string reason_phrase;
    std::string res;

    reason_phrase = getReasonPhrase(code);
    if (reason_phrase.empty()) {
        reason_phrase = "Internal Server Error";
        error_code = "500";
    } else
        error_code = std::to_string(code);
    std::string path = server.getCustomErrorPagePath(code);
    setResponseString("HTTP/1.1", error_code, reason_phrase);
    if (!path.empty()) {
        short i = writeFileToBuffer(path);
        if (i == 200)
            return;
    }
    res = getErrorHtml(error_code, reason_phrase);
    _body.insert(_body.end(), res.begin(), res.end());
    _body_size = _body.size();
}

//error response constructor
HttpResponse::HttpResponse(short n, const VirtualServer &server) {
    setError(n, server);
}

HttpResponse::HttpResponse(short n, const VirtualServer &server, const Location &loc) {
    setError(n, server);
    if (n == 405)
        insertHeader("Allow", loc.getAllowedMethodsField());
}

HttpResponse::HttpResponse(const HttpRequest &request, std::string &abs_path, const VirtualServer &server,
                           const Location &loc) : _absolute_path(abs_path) {
    short err;

    _request = request;
    if (request.getMethod() == "DELETE") {
        if (!Utils::fileExistsAndWritable(abs_path)) {
            if (errno == EACCES)
                setError(403, server);
            else
                setError(410, server);
        } else {
            if (std::remove(abs_path.data()) != 0)
                setError(500, server);
            else
                setResponseString("HTTP/1.1", "200", "OK");
        }
        return;
    }

    if (!loc.getCgiPass().empty()) {
        prepareCgiEnv(request, abs_path, server.getPort());
        _cgi_path = loc.getCgiPass();
        return;
    }
    if (request.getMethod() == "POST") {
        if (loc.isFileUploadOn()) {

            int num_files = Utils::countFilesInFolder(loc.getFileUploadPath());
            std::ofstream rf("uploaded_file" + std::to_string(num_files + 1), std::ios::out | std::ios::binary);
            if (!rf) {
                setError(500, server);
                return;
            }
            else
            {
                rf.write(request.getBody().data(), request.getBody().size());
                rf.close();
                setResponseString("HTTP/1.1", "200", "OK");
                return;
            }
        } else {
            setError(404, server);
            return;
        }
    }
    if (request.getMethod() == "GET") {
        err = writeFileToBuffer(abs_path);
        if (err == 200) {
            setResponseString("HTTP/1.1", "200", "OK");
            return;
        } else {
            setError(err, server);
            return;
        }
    } else
        setError(900, server);
}

void
HttpResponse::prepareCgiEnv(HttpRequest const &request, const std::string &absolute_path, const uint16_t serv_port) {
    _cgi_env["AUTH_TYPE"] = "";
    if (!request.getBody().empty()) {
        _cgi_env["CONTENT_LENGTH"] = std::to_string(request.getBody().size());
        std::map<std::string, std::string>::const_iterator it = request.getHeaderFields().find("Content-Type");
        if (it != request.getHeaderFields().end())
            _cgi_env["CONTENT_TYPE"] = it->second;
    }
    _cgi_env["GATEWAY_INTERFACE"] = std::string("CGI/1.1");
    std::string path_info = absolute_path.substr(absolute_path.find_last_of('/') + 1);
    if (path_info.empty())
        _cgi_env["PATH_TRANSLATED"] = "";
    else
        _cgi_env["PATH_TRANSLATED"] = absolute_path; // @todo GET ABSOLUTE PATH
//	_cgi_env["PATH_TRANSLATED"] = "/Users/megagosha/42/webserv/www/final"; // @todo GET ABSOLUTE PATH
    _cgi_env["SCRIPT_FILENAME"] = absolute_path;
    _cgi_env["QUERY_STRING"] = request.getQueryString();
    _cgi_env["REMOTE_ADDR"] = request.getClientIp();
    _cgi_env["REMOTE_HOST"] = "";
    _cgi_env["REMOTE_IDENT"] = "";
    _cgi_env["REMOTE_USER"] = "";
    _cgi_env["REQUEST_METHOD"] = request.getMethod();
    _cgi_env["PATH_INFO"] = absolute_path;
    _cgi_env["SCRIPT_NAME"] = request.getRequestUri();
    std::map<std::string, std::string>::const_iterator it = request.getHeaderFields().find("Host");
    if (it == request.getHeaderFields().end())
        _cgi_env["SERVER_NAME"] = ""; //@todo check logic Maybe get server name from VirtualServer config
    else
        _cgi_env["SERVER_NAME"] = it->second;
    _cgi_env["SERVER_PORT"] = std::to_string(serv_port); // + '\0';
    _cgi_env["SERVER_PROTOCOL"] = "HTTP/1.1";
    _cgi_env["SERVER_SOFTWARE"] = "topserv_v0.1";
}

int HttpResponse::executeCgi() {
    int fd[2];
    char **envp;
    char tmp[64001]; //store current path
    char **argv = new char *[2];
    pid_t nChild;
    int status;
    int aStdinPipe[2];
    int aStdoutPipe[2];
    int nResult;
    char *str;
    std::map<std::string, std::string>::iterator it;


    getcwd(tmp, 500);
    pipe(fd);
    it = _cgi_env.find("SCRIPT_FILENAME");
    argv[0] = new char[it->second.size() + 1];
    argv[1] = nullptr;
    strcpy(argv[0], it->second.data());

    std::string dir = it->second.substr(0, it->second.find_last_of('/'));
    //	chdir(dir.data()); //@todo support relative paths
    if (pipe(aStdinPipe) < 0) {
        std::cout << "allocating pipe for child input redirect" << std::endl;
        return (500);
    }
    if (pipe(aStdoutPipe) < 0) {
        close(aStdinPipe[PIPE_READ]);
        close(aStdinPipe[PIPE_WRITE]);
        std::cout << "allocating pipe for child output redirect" << std::endl;
        return (500);
    }

    str = new char[_cgi_path.size() + 1];
    strcpy(str, _cgi_path.data());

    envp = Utils::mapToEnv(_cgi_env);

    nChild = fork();
    if (0 == nChild) {
        if (dup2(aStdinPipe[PIPE_READ], STDIN_FILENO) == -1)
            exit(errno);
        if (dup2(aStdoutPipe[PIPE_WRITE], STDOUT_FILENO) == -1)
            exit(errno);
        if (dup2(aStdoutPipe[PIPE_WRITE], STDERR_FILENO) == -1)
            exit(errno);

        // all these are for use by parent only
        close(aStdinPipe[PIPE_READ]);
        close(aStdinPipe[PIPE_WRITE]);
        close(aStdoutPipe[PIPE_READ]);
        close(aStdoutPipe[PIPE_WRITE]);

        nResult = execve(str, argv, envp);
        exit(nResult);
    } else if (nChild > 0) {
        Utils::clearNullArr(envp);
        close(aStdinPipe[PIPE_READ]);
        if (!_request.getBody().empty())
        {
            write(aStdinPipe[PIPE_WRITE], _request.getBody().data(), _request.getBody().size());
        }
        close(aStdoutPipe[PIPE_WRITE]);
        //buffer size is limited. so we should try reading while child process is alive;
        pid_t rc_pid = 500;

        int i;
        KqueueEvents kq(1);
        kq.addFd(aStdoutPipe[PIPE_READ]);
//		kq.addProcess(nChild);
        std::pair<int, struct kevent *> res;
        while (true) {
            res = kq.getUpdates(15); // 10 sec timeout
            std::cout << res.first << std::endl;
            if (res.first == 0) {
                waitpid(nChild, &status, WNOHANG);
                rc_pid = 500;
                break;
            } else if (res.second[0].filter == EVFILT_READ) {
                i = read(aStdoutPipe[PIPE_READ], &tmp, 64000);
                if (i > 0)
                    _body.insert(_body.end(), tmp, tmp + i);
                else if (i < 0) {
                    rc_pid = 500;
                    break;
                }
                if (res.second[0].flags & EV_EOF) {
                    waitpid(nChild, &status, WNOHANG);
                    if (WIFEXITED(status)) {
                        if (WEXITSTATUS(status) == 0)
                            rc_pid = 200;
                        else
                            rc_pid = 500;
                        break;
                    }
                    rc_pid = 500;
                    break;
                }
            } else if (res.second[0].flags & EV_ERROR) {
                waitpid(nChild, &status, WNOHANG);
                rc_pid = 500;
            }
        }
        //@todo change dir back for relative path support
        return (rc_pid);
    } else { // failed to create child
        Utils::clearNullArr(envp);
        close(aStdinPipe[PIPE_READ]);
        close(aStdinPipe[PIPE_WRITE]);
        close(aStdoutPipe[PIPE_READ]);
        close(aStdoutPipe[PIPE_WRITE]);
        return (500);
    }
}

int HttpResponse::sendResponse(int fd) {
    std::map<std::string, std::string>::iterator it = _header.begin();
    std::vector<char> headers_vec;
    short res = 200;

    if (!_cgi_path.empty()) {
        res = executeCgi();
        setResponseString("HTTP/1.1", std::to_string(res), getReasonPhrase(res));
        //@todo check logic Should client always receive cgi errors?
        if (res != 200) {
            VirtualServer s;
            setError(res, s); // @todo support custom error pages here
            _cgi_path.clear();
        }
    }
    setTimeHeader();
    if (_body_size > 0 && _cgi_path.empty())
        insertHeader("Content-Length", std::to_string(_body.size()));
    headers_vec.reserve(500);
    headers_vec.insert(headers_vec.end(), _response_string.begin(), _response_string.end());
    for (it = _header.begin(); it != _header.end(); ++it) {
        headers_vec.insert(headers_vec.end(), it->first.begin(), it->first.end());
        headers_vec.push_back(':');
        headers_vec.push_back(' ');
        headers_vec.insert(headers_vec.end(), it->second.begin(), it->second.end());
        headers_vec.push_back('\r');
        headers_vec.push_back('\n');
    }
    if (_cgi_path.empty()) {
        headers_vec.push_back('\r');
        headers_vec.push_back('\n');
    }

    write(STDOUT_FILENO, headers_vec.data(), headers_vec.size());
    if (_body_size > 0)
        write(STDOUT_FILENO, _body.data(), _body.size());
    send(fd, headers_vec.data(), headers_vec.size(), 0);
    if (_body_size > 0)
        send(fd, _body.data(), _body.size(), 0);
    return (EXIT_SUCCESS);
}

HttpResponse::HttpResponse(const HttpResponse &rhs) :
        _proto(rhs._proto),
        _status_code(rhs._status_code),
        _status_reason(rhs._status_reason),
        _response_string(rhs._response_string),
        _header(rhs._header),
        _body(rhs._body),
        _body_size(rhs._body_size),
        _cgi_env(rhs._cgi_env),
        _cgi_path(rhs._cgi_path) {
};

HttpResponse &HttpResponse::operator=(const HttpResponse &rhs) {
    if (this == &rhs)
        return (*this);
    _proto = rhs.getProto();
    _status_code = rhs.getStatusCode();
    _status_reason = rhs.getStatusReason();
    _response_string = rhs.getResponseString();
    _header = rhs.getHeader();
    _body = rhs.getBody();
    _body_size = rhs.getBodySize();
    _cgi_env = rhs._cgi_env;
    _cgi_path = rhs._cgi_path;
    return (*this);
}

const std::string &HttpResponse::getProto() const {
    return _proto;
}

const std::string &HttpResponse::getStatusCode() const {
    return _status_code;
}

const std::string &HttpResponse::getStatusReason() const {
    return _status_reason;
}

const std::string &HttpResponse::getResponseString() const {
    return _response_string;
}

const std::map<std::string, std::string> &HttpResponse::getHeader() const {
    return _header;
}

const std::vector<char> &HttpResponse::getBody() const {
    return _body;
}

size_t HttpResponse::getBodySize() const {
    return _body_size;
}

const std::string &HttpResponse::getAbsolutePath() const {
    return _absolute_path;
}


HttpResponse::~HttpResponse() {
}