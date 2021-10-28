//
// Created by Elayne Debi on 9/9/21.
//

#include "HttpResponse.hpp"

HttpResponse::HttpResponse() {
}

HttpResponse::~HttpResponse() {
}

//error response constructor
HttpResponse::HttpResponse(HTTPStatus code, const VirtualServer *server) {
    setError(code, server);
}


/*
 * A server MUST NOT send a Content-Length header field in
 * any response with a status code of 1xx (Informational) or 204 (No Content).
 * A server MUST NOT send a Content-Length header field in any 2xx (Successful)
 * response to a CONNECT request (Section 4.3.6 of [RFC7231]).

 */
HttpResponse::HttpResponse(Session &session, const VirtualServer *config) {
    const Location *loc;
    HttpRequest    *req = session.getRequest();

    if (config == nullptr) {
        setError(HTTP_BAD_REQUEST, config);
        return;
    }
    if (session.getRequest()->getParsingError() != 0) {
        setError((HTTPStatus) session.getRequest()->getParsingError(), config);
        return;
    }
    loc = config->getLocationFromRequest(*req);
    if (loc == nullptr)
        setError(HTTP_NOT_FOUND, config);
    if (!loc->methodAllowed(req->getMethod())) {
        insertHeader("Allow", loc->getAllowedMethodsField());
        setError(HTTP_METHOD_NOT_ALLOWED, config);
        return;
    }
    if (loc->isMaxBodySet() && loc->getMaxBody() < req->getBody().size())
    {
    	session.setKeepAlive(false);
		setError(HTTP_REQUEST_ENTITY_TOO_LARGE, config);
		return;
    }

//	std::map<std::string, std::string>::const_iterator it = req->getHeaderFields().find("Expected");
//	if (it != req->getHeaderFields().end() && it->second == "100-continue")
//	{
//		//@todo check size requirements
//		setError(HTTP_EXPECTATION_FAILED, config);
//		return;
//	}
    if (!loc->getRet().empty()) {
        insertHeader("Location", loc->getRet());
        setResponseString("HTTP/1.1", HTTP_MOVED_PERMANENTLY);
        return;
    }
    if (!loc->getCgiPass().empty()) {
        prepareCgiEnv(*req, req->getNormalizedPath(), config->getPort());
        _cgi_path = loc->getCgiPass();
        return;
    } else if (req->getMethod() == "GET")
        processGetRequest(config, loc, req);
    else if (req->getMethod() == "POST")
        processPostRequest(config);
    else if (req->getMethod() == "DELETE")
        processDeleteRequest(config, req);
    else if (req->getMethod() == "PUT")
        processPutRequest(config, loc, req);
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
    _proto           = rhs.getProto();
    _status_code     = rhs.getStatusCode();
    _status_reason   = rhs.getStatusReason();
    _response_string = rhs.getResponseString();
    _header          = rhs.getHeader();
    _body            = rhs.getBody();
    _body_size       = rhs.getBodySize();
    _cgi_env         = rhs._cgi_env;
    _cgi_path        = rhs._cgi_path;
    return (*this);
}


void
HttpResponse::processGetRequest(const VirtualServer *serv, const Location *loc, HttpRequest *req) {
    if (Utils::isDirectory(req->getNormalizedPath())) {
        if (loc->isAutoindexOn()) {
            getAutoIndex(req->getNormalizedPath(), req->getUriNoQuery());
            return;
        }
    }
    HTTPStatus err = writeFileToBuffer(req->getNormalizedPath());
    if (err == HTTP_OK) {
        setResponseString("HTTP/1.1", HTTP_OK);
        return;
    } else {
        setError(err, serv);
        return;
    }
}

void
HttpResponse::processPutRequest(const VirtualServer *serv, const Location *loc, HttpRequest *req) {
//    std::map<std::string, std::string>::const_iterator it;
    std::string                                        file_name;

    file_name = Utils::getFileNameFromRequest(req->getRequestUri());

    if (!loc->isFileUploadOn()) {
        setError(HTTP_METHOD_NOT_ALLOWED, serv);
        return;
    }
//    it = req->getHeaderFields().find("Content-Type");
    if (!Utils::checkIfPathExists(req->getNormalizedPath()) || file_name.empty()) {
        setError(HTTP_NOT_FOUND, serv);
        return;
    }
    if (Utils::fileExistsAndWritable(req->getNormalizedPath()))
        setResponseString("HTTP/1.1", HTTP_NO_CONTENT);
    else
        setResponseString("HTTP/1.1", HTTP_CREATED);
    std::ofstream rf(req->getNormalizedPath(), std::ios::out | std::ios::binary);

//    if (it != req->getHeaderFields().end())
//        extension = MimeType::getFileExtension(it->second);
//    num_files     = Utils::countFilesInFolder(loc->getFileUploadPath());
//    std::ofstream rf(loc->getFileUploadPath() + "uploaded_file" + std::to_string(num_files + 1) + "." + extension,
//                     std::ios::out | std::ios::binary);
    if (rf) {
        rf.write(req->getBody().data(), req->getBody().size());
        rf.close();
        insertHeader("Content-Location", loc->getPath() + file_name);
        return;
    } else {
        setError(HTTP_INTERNAL_SERVER_ERROR, serv);
        return;
    }
}

void
HttpResponse::processPostRequest(const VirtualServer *serv) {
//        setError(HTTP_METHOD_NOT_ALLOWED, serv);
	if (serv == nullptr)
	{
		setError(HTTP_BAD_REQUEST, nullptr);
		return;
	}
	setResponseString("HTTP/1.1", HTTP_OK);
	_body_size = 0;
}

void HttpResponse::processDeleteRequest(const VirtualServer *conf,
                                        HttpRequest *req) {
    if (Utils::isNotEmptyDirectory(req->getNormalizedPath())) {
        setError(HTTP_CONFLICT, conf);
        return;
    }
    if (!Utils::fileExistsAndWritable(req->getNormalizedPath())) {
        if (errno == EACCES)
            setError(HTTP_FORBIDDEN, conf);
        else
            setError(HTTP_GONE, conf);
    } else {
        if (std::remove(req->getNormalizedPath().data()) != 0)
            setError(HTTP_INTERNAL_SERVER_ERROR, conf);
        else
            setResponseString("HTTP/1.1", HTTP_OK);
    }
    return;
}

void HttpResponse::setResponseString(const std::string &pr, HTTPStatus status) {
    _proto           = pr;
    _status_code     = status;
    _status_reason   = getReasonForStatus(status);
    _response_string = _proto + " " + std::to_string(_status_code) + " " + _status_reason + "\r\n";
}

HttpResponse::HTTPStatus HttpResponse::writeFileToBuffer(const std::string &file_path) {
    long long int length;
    std::ifstream file(file_path, std::ifstream::in | std::ifstream::binary);
    if (file) {
        file.seekg(0, file.end);
        length = file.tellg();
        if (length <= 0) {
            _body_size = 0;
            return (HTTP_INTERNAL_SERVER_ERROR);
        }
        file.seekg(0, file.beg);
        _body_size = (std::size_t) length;
        _body.reserve(_body.size() + length);
        _body.insert(_body.end(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        if (file.peek() != EOF) {
            _body_size = 0;
            return (HTTP_INTERNAL_SERVER_ERROR);
        }
        file.close();
        insertHeader("Content-Type", MimeType::getType(file_path));//@todo determine content type
        return (HTTP_OK);
    } else
        return (HTTP_NOT_FOUND);
}

void HttpResponse::insertHeader(std::string name, std::string value) {
	_header[name] = value;
}

void HttpResponse::setTimeHeader(void) {
    time_t now = time(nullptr);
    char   buf[100];

    std::strftime(buf, 100, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&now));
    insertHeader("Date", buf);
    insertHeader("Server", "mg_webserv/0.01");
}

std::string HttpResponse::getErrorHtml(std::string &error, std::string &reason) {
    std::string res;

    res += "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"/><title>" + error +
           "</title></head><body><h1>" + error + " " + reason + "</h1></body></html>\n";
    return (res);
}


void HttpResponse::setError(HTTPStatus code, const VirtualServer *server) {
    std::string error_code = std::to_string(code);
    std::string reason_phrase;
    std::string res;


    reason_phrase = getReasonForStatus(code);

    std::string path;
    if (server != nullptr)
        server->getCustomErrorPagePath(code);
    setResponseString("HTTP/1.1", code);
    if (code == HTTP_REQUEST_TIMEOUT)
    {
    	insertHeader("Connection", "close");
    	_header.erase("Keep-Alive");
    }
    if (!path.empty()) {
        short i = writeFileToBuffer(path);
        if (i == 200)
            return;
    }
    res = getErrorHtml(error_code, reason_phrase);
    _body.insert(_body.end(), res.begin(), res.end());
    _body_size = _body.size();
}

void
HttpResponse::prepareCgiEnv(HttpRequest const &request, const std::string &absolute_path, const uint16_t serv_port) {
    _cgi_env["AUTH_TYPE"]         = "";
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
    _cgi_env["SCRIPT_FILENAME"]     = absolute_path;
    _cgi_env["QUERY_STRING"]        = request.getQueryString();
    _cgi_env["REMOTE_ADDR"]         = request.getClientIp();
    _cgi_env["REMOTE_HOST"]         = "";
    _cgi_env["REMOTE_IDENT"]        = "";
    _cgi_env["REMOTE_USER"]         = "";
    _cgi_env["REQUEST_METHOD"]      = request.getMethod();
    _cgi_env["PATH_INFO"]           = absolute_path;
    _cgi_env["SCRIPT_NAME"]         = request.getRequestUri();
    std::map<std::string, std::string>::const_iterator it = request.getHeaderFields().find("Host");
    if (it == request.getHeaderFields().end())
        _cgi_env["SERVER_NAME"] = ""; //@todo check logic Maybe get server name from VirtualServer config
    else
        _cgi_env["SERVER_NAME"] = it->second;
    _cgi_env["SERVER_PORT"]     = std::to_string(serv_port); // + '\0';
    _cgi_env["SERVER_PROTOCOL"] = "HTTP/1.1";
    _cgi_env["SERVER_SOFTWARE"] = "topserv_v0.1";
}


//@todo rewrite
HttpResponse::HTTPStatus HttpResponse::executeCgi(HttpRequest *req) {
    int                                          fd[2];
    char                                         **envp;
    char                                         tmp[64001]; //store current path
    char                                         **argv = new char *[2];
    pid_t                                        nChild;
    int                                          status;
    int                                          aStdinPipe[2];
    int                                          aStdoutPipe[2];
    int                                          nResult;
    char                                         *str;
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
        return (HTTP_INTERNAL_SERVER_ERROR);
    }
    if (pipe(aStdoutPipe) < 0) {
        close(aStdinPipe[PIPE_READ]);
        close(aStdinPipe[PIPE_WRITE]);
        std::cout << "allocating pipe for child output redirect" << std::endl;
        return (HTTP_INTERNAL_SERVER_ERROR);
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
        HTTPStatus http_status = HTTP_INTERNAL_SERVER_ERROR;
        Utils::clearNullArr(envp);
        close(aStdinPipe[PIPE_READ]);
        if (!req->getBody().empty()) {
            write(aStdinPipe[PIPE_WRITE], req->getBody().data(), req->getBody().size());
        }
        close(aStdoutPipe[PIPE_WRITE]);
        //buffer size is limited. so we should try reading while child process is alive;

        int          i;
        KqueueEvents kq(1);
        kq.addFd(aStdoutPipe[PIPE_READ]);
//		kq.addProcess(nChild);
        std::pair<int, struct kevent *> res;
        while (true) {
            res = kq.getUpdates(15); // 10 sec timeout
            std::cout << res.first << std::endl;
            if (res.first == 0) {
                waitpid(nChild, &status, WNOHANG);
                break;
            } else if (res.second[0].filter == EVFILT_READ) {
                i = read(aStdoutPipe[PIPE_READ], &tmp, 64000);
                if (i > 0)
                    _body.insert(_body.end(), tmp, tmp + i);
                else if (i < 0) {
                    break;
                }
                if (res.second[0].flags & EV_EOF) {
                    waitpid(nChild, &status, WNOHANG);
                    if (WIFEXITED(status)) {
                        if (WEXITSTATUS(status) == 0)
                            http_status = HTTP_OK;
                        break;
                    }
                    break;
                }
            } else if (res.second[0].flags & EV_ERROR) {
                waitpid(nChild, &status, WNOHANG);
            }
        }
        //@todo change dir back for relative path support
        return (http_status);
    } else { // failed to create child
        Utils::clearNullArr(envp);
        close(aStdinPipe[PIPE_READ]);
        close(aStdinPipe[PIPE_WRITE]);
        close(aStdoutPipe[PIPE_READ]);
        close(aStdoutPipe[PIPE_WRITE]);
        return (HTTP_INTERNAL_SERVER_ERROR);
    }
}

int HttpResponse::sendResponse(int fd, HttpRequest *req) {
    std::map<std::string, std::string>::iterator it  = _header.begin();
    std::vector<char>                            headers_vec;
    HTTPStatus                                   res = HTTP_OK;

//	if (req->getParsingError() == 0)

    if (!_cgi_path.empty() && req != nullptr) {
        res = executeCgi(req);
        setResponseString("HTTP/1.1", res);
        //@todo check logic Should client always receive cgi errors?
        if (res != 200) {
            setError(res, nullptr); // @todo support custom error pages here
            _cgi_path.clear();
        }
    }
    setTimeHeader();
    if (_body_size > 0 && _cgi_path.empty())
        insertHeader("Content-Length", std::to_string(_body.size()));
    headers_vec.reserve(500);
    headers_vec.insert(headers_vec.begin(), _response_string.begin(), _response_string.end());
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

//	write(STDOUT_FILENO, headers_vec.data(), headers_vec.size());
//	if (_body_size > 0)
//		write(STDOUT_FILENO, _body.data(), _body.size());
std::cout << _response_string << std::endl;
    send(fd, headers_vec.data(), headers_vec.size(), 0);
    write(STDOUT_FILENO, headers_vec.data(), headers_vec.size());
    if (_body_size > 0)
        send(fd, _body.data(), _body.size(), 0);
    if (_body_size > 0)
    	write(STDOUT_FILENO, _body.data(), _body.size());
    return (EXIT_SUCCESS);
}


const std::string &HttpResponse::getProto() const {
    return _proto;
}

uint16_t HttpResponse::getStatusCode() const {
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

void HttpResponse::insertTableIntoBody(const std::string &str, const std::string &uri) {
    size_t pos = AUTOINDEX_HTML.find("<body>");

    pos += 6;
    _body.assign(AUTOINDEX_HTML.begin(), AUTOINDEX_HTML.end());
    _body.insert(_body.begin() + pos, str.begin(), str.end());
    _body.insert(_body.begin() + 66, uri.begin(), uri.end());
    _body_size = _body.size();
    insertHeader("Content-Type", "text/html");
    setResponseString("HTTP/1.1", HTTP_OK);
}

void HttpResponse::getAutoIndex(const std::string &path, const std::string &uri_path) {

    DIR           *dp;
    struct dirent *di_struct;
    int           i = 0;
    std::string   table;

    dp = opendir(path.data());
    table += "<h1>" + uri_path + "</h1>";
    table += "<table>";
    table += "<tr> <th>File name</th> <th>File size</th> <th>Last modified</th> </tr>";
    if (dp != NULL) {
        while ((di_struct = readdir(dp)) != nullptr) {
            FileStats file(path + "/" + di_struct->d_name);
            table += "<tr>";

            table += "<td><a href=\"" + uri_path;
            table += di_struct->d_name;
            if (file.isDir())
                table += "/";
            table += "\">" + std::string(di_struct->d_name) + "</a></td>";
            table += "<td>" + file.getSizeInMb() + "</td>";
            table += "<td>" + file.getTimeModified() + "</td>";
            table += "</tr>";
            i++;
        }
        closedir(dp);
    }
    table += "</table>";
    insertTableIntoBody(table, uri_path);
}

const std::string &HttpResponse::getReasonForStatus(HTTPStatus status) {
    switch (status) {
        case HTTP_CONTINUE:
            return HTTP_REASON_CONTINUE;
        case HTTP_SWITCHING_PROTOCOLS:
            return HTTP_REASON_SWITCHING_PROTOCOLS;
        case HTTP_PROCESSING:
            return HTTP_REASON_PROCESSING;
        case HTTP_OK:
            return HTTP_REASON_OK;
        case HTTP_CREATED:
            return HTTP_REASON_CREATED;
        case HTTP_ACCEPTED:
            return HTTP_REASON_ACCEPTED;
        case HTTP_NONAUTHORITATIVE:
            return HTTP_REASON_NONAUTHORITATIVE;
        case HTTP_NO_CONTENT:
            return HTTP_REASON_NO_CONTENT;
        case HTTP_RESET_CONTENT:
            return HTTP_REASON_RESET_CONTENT;
        case HTTP_PARTIAL_CONTENT:
            return HTTP_REASON_PARTIAL_CONTENT;
        case HTTP_MULTI_STATUS:
            return HTTP_REASON_MULTI_STATUS;
        case HTTP_ALREADY_REPORTED:
            return HTTP_REASON_ALREADY_REPORTED;
        case HTTP_IM_USED:
            return HTTP_REASON_IM_USED;
        case HTTP_MULTIPLE_CHOICES:
            return HTTP_REASON_MULTIPLE_CHOICES;
        case HTTP_MOVED_PERMANENTLY:
            return HTTP_REASON_MOVED_PERMANENTLY;
        case HTTP_FOUND:
            return HTTP_REASON_FOUND;
        case HTTP_SEE_OTHER:
            return HTTP_REASON_SEE_OTHER;
        case HTTP_NOT_MODIFIED:
            return HTTP_REASON_NOT_MODIFIED;
        case HTTP_USE_PROXY:
            return HTTP_REASON_USE_PROXY;
        case HTTP_TEMPORARY_REDIRECT:
            return HTTP_REASON_TEMPORARY_REDIRECT;
        case HTTP_BAD_REQUEST:
            return HTTP_REASON_BAD_REQUEST;
        case HTTP_UNAUTHORIZED:
            return HTTP_REASON_UNAUTHORIZED;
        case HTTP_PAYMENT_REQUIRED:
            return HTTP_REASON_PAYMENT_REQUIRED;
        case HTTP_FORBIDDEN:
            return HTTP_REASON_FORBIDDEN;
        case HTTP_NOT_FOUND:
            return HTTP_REASON_NOT_FOUND;
        case HTTP_METHOD_NOT_ALLOWED:
            return HTTP_REASON_METHOD_NOT_ALLOWED;
        case HTTP_NOT_ACCEPTABLE:
            return HTTP_REASON_NOT_ACCEPTABLE;
        case HTTP_PROXY_AUTHENTICATION_REQUIRED:
            return HTTP_REASON_PROXY_AUTHENTICATION_REQUIRED;
        case HTTP_REQUEST_TIMEOUT:
            return HTTP_REASON_REQUEST_TIMEOUT;
        case HTTP_CONFLICT:
            return HTTP_REASON_CONFLICT;
        case HTTP_GONE:
            return HTTP_REASON_GONE;
        case HTTP_LENGTH_REQUIRED:
            return HTTP_REASON_LENGTH_REQUIRED;
        case HTTP_PRECONDITION_FAILED:
            return HTTP_REASON_PRECONDITION_FAILED;
        case HTTP_REQUEST_ENTITY_TOO_LARGE:
            return HTTP_REASON_REQUEST_ENTITY_TOO_LARGE;
        case HTTP_REQUEST_URI_TOO_LONG:
            return HTTP_REASON_REQUEST_URI_TOO_LONG;
        case HTTP_UNSUPPORTED_MEDIA_TYPE:
            return HTTP_REASON_UNSUPPORTED_MEDIA_TYPE;
        case HTTP_REQUESTED_RANGE_NOT_SATISFIABLE:
            return HTTP_REASON_REQUESTED_RANGE_NOT_SATISFIABLE;
        case HTTP_EXPECTATION_FAILED:
            return HTTP_REASON_EXPECTATION_FAILED;
        case HTTP_IM_A_TEAPOT:
            return HTTP_REASON_IM_A_TEAPOT;
        case HTTP_ENCHANCE_YOUR_CALM:
            return HTTP_REASON_ENCHANCE_YOUR_CALM;
        case HTTP_MISDIRECTED_REQUEST:
            return HTTP_REASON_MISDIRECTED_REQUEST;
        case HTTP_UNPROCESSABLE_ENTITY:
            return HTTP_REASON_UNPROCESSABLE_ENTITY;
        case HTTP_LOCKED:
            return HTTP_REASON_LOCKED;
        case HTTP_FAILED_DEPENDENCY:
            return HTTP_REASON_FAILED_DEPENDENCY;
        case HTTP_UPGRADE_REQUIRED:
            return HTTP_REASON_UPGRADE_REQUIRED;
        case HTTP_PRECONDITION_REQUIRED:
            return HTTP_REASON_PRECONDITION_REQUIRED;
        case HTTP_TOO_MANY_REQUESTS:
            return HTTP_REASON_TOO_MANY_REQUESTS;
        case HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE:
            return HTTP_REASON_REQUEST_HEADER_FIELDS_TOO_LARGE;
        case HTTP_UNAVAILABLE_FOR_LEGAL_REASONS:
            return HTTP_REASON_UNAVAILABLE_FOR_LEGAL_REASONS;
        case HTTP_INTERNAL_SERVER_ERROR:
            return HTTP_REASON_INTERNAL_SERVER_ERROR;
        case HTTP_NOT_IMPLEMENTED:
            return HTTP_REASON_NOT_IMPLEMENTED;
        case HTTP_BAD_GATEWAY:
            return HTTP_REASON_BAD_GATEWAY;
        case HTTP_SERVICE_UNAVAILABLE:
            return HTTP_REASON_SERVICE_UNAVAILABLE;
        case HTTP_GATEWAY_TIMEOUT:
            return HTTP_REASON_GATEWAY_TIMEOUT;
        case HTTP_VERSION_NOT_SUPPORTED:
            return HTTP_REASON_VERSION_NOT_SUPPORTED;
        case HTTP_VARIANT_ALSO_NEGOTIATES:
            return HTTP_REASON_VARIANT_ALSO_NEGOTIATES;
        case HTTP_INSUFFICIENT_STORAGE:
            return HTTP_REASON_INSUFFICIENT_STORAGE;
        case HTTP_LOOP_DETECTED:
            return HTTP_REASON_LOOP_DETECTED;
        case HTTP_NOT_EXTENDED:
            return HTTP_REASON_NOT_EXTENDED;
        case HTTP_NETWORK_AUTHENTICATION_REQUIRED:
            return HTTP_REASON_NETWORK_AUTHENTICATION_REQUIRED;
        default:
            return HTTP_REASON_UNKNOWN;
    }
}

const std::string HttpResponse::HTTP_REASON_CONTINUE                        = "Continue";
const std::string HttpResponse::HTTP_REASON_SWITCHING_PROTOCOLS             = "Switching Protocols";
const std::string HttpResponse::HTTP_REASON_PROCESSING                      = "Processing";
const std::string HttpResponse::HTTP_REASON_OK                              = "OK";
const std::string HttpResponse::HTTP_REASON_CREATED                         = "Created";
const std::string HttpResponse::HTTP_REASON_ACCEPTED                        = "Accepted";
const std::string HttpResponse::HTTP_REASON_NONAUTHORITATIVE                = "Non-Authoritative Information";
const std::string HttpResponse::HTTP_REASON_NO_CONTENT                      = "No Content";
const std::string HttpResponse::HTTP_REASON_RESET_CONTENT                   = "Reset Content";
const std::string HttpResponse::HTTP_REASON_PARTIAL_CONTENT                 = "Partial Content";
const std::string HttpResponse::HTTP_REASON_MULTI_STATUS                    = "Multi Status";
const std::string HttpResponse::HTTP_REASON_ALREADY_REPORTED                = "Already Reported";
const std::string HttpResponse::HTTP_REASON_IM_USED                         = "IM Used";
const std::string HttpResponse::HTTP_REASON_MULTIPLE_CHOICES                = "Multiple Choices";
const std::string HttpResponse::HTTP_REASON_MOVED_PERMANENTLY               = "Moved Permanently";
const std::string HttpResponse::HTTP_REASON_FOUND                           = "Found";
const std::string HttpResponse::HTTP_REASON_SEE_OTHER                       = "See Other";
const std::string HttpResponse::HTTP_REASON_NOT_MODIFIED                    = "Not Modified";
const std::string HttpResponse::HTTP_REASON_USE_PROXY                       = "Use Proxy";
const std::string HttpResponse::HTTP_REASON_TEMPORARY_REDIRECT              = "Temporary Redirect";
const std::string HttpResponse::HTTP_REASON_PERMANENT_REDIRECT              = "Permanent Redirect";
const std::string HttpResponse::HTTP_REASON_BAD_REQUEST                     = "Bad Request";
const std::string HttpResponse::HTTP_REASON_UNAUTHORIZED                    = "Unauthorized";
const std::string HttpResponse::HTTP_REASON_PAYMENT_REQUIRED                = "Payment Required";
const std::string HttpResponse::HTTP_REASON_FORBIDDEN                       = "Forbidden";
const std::string HttpResponse::HTTP_REASON_NOT_FOUND                       = "Not Found";
const std::string HttpResponse::HTTP_REASON_METHOD_NOT_ALLOWED              = "Method Not Allowed";
const std::string HttpResponse::HTTP_REASON_NOT_ACCEPTABLE                  = "Not Acceptable";
const std::string HttpResponse::HTTP_REASON_PROXY_AUTHENTICATION_REQUIRED   = "Proxy Authentication Required";
const std::string HttpResponse::HTTP_REASON_REQUEST_TIMEOUT                 = "Request Time-out";
const std::string HttpResponse::HTTP_REASON_CONFLICT                        = "Conflict";
const std::string HttpResponse::HTTP_REASON_GONE                            = "Gone";
const std::string HttpResponse::HTTP_REASON_LENGTH_REQUIRED                 = "Length Required";
const std::string HttpResponse::HTTP_REASON_PRECONDITION_FAILED             = "Precondition Failed";
const std::string HttpResponse::HTTP_REASON_REQUEST_ENTITY_TOO_LARGE        = "Request Entity Too Large";
const std::string HttpResponse::HTTP_REASON_REQUEST_URI_TOO_LONG            = "Request-URI Too Large";
const std::string HttpResponse::HTTP_REASON_UNSUPPORTED_MEDIA_TYPE          = "Unsupported Media Type";
const std::string HttpResponse::HTTP_REASON_REQUESTED_RANGE_NOT_SATISFIABLE = "Requested Range Not Satisfiable";
const std::string HttpResponse::HTTP_REASON_EXPECTATION_FAILED              = "Expectation Failed";
const std::string HttpResponse::HTTP_REASON_IM_A_TEAPOT                     = "I'm a Teapot";
const std::string HttpResponse::HTTP_REASON_ENCHANCE_YOUR_CALM              = "Enchance Your Calm";
const std::string HttpResponse::HTTP_REASON_MISDIRECTED_REQUEST             = "Misdirected Request";
const std::string HttpResponse::HTTP_REASON_UNPROCESSABLE_ENTITY            = "Unprocessable Entity";
const std::string HttpResponse::HTTP_REASON_LOCKED                          = "Locked";
const std::string HttpResponse::HTTP_REASON_FAILED_DEPENDENCY               = "Failed Dependency";
const std::string HttpResponse::HTTP_REASON_UPGRADE_REQUIRED                = "Upgrade Required";
const std::string HttpResponse::HTTP_REASON_PRECONDITION_REQUIRED           = "Precondition Required";
const std::string HttpResponse::HTTP_REASON_TOO_MANY_REQUESTS               = "Too Many Requests";
const std::string HttpResponse::HTTP_REASON_REQUEST_HEADER_FIELDS_TOO_LARGE = "Request Header Fields Too Large";
const std::string HttpResponse::HTTP_REASON_UNAVAILABLE_FOR_LEGAL_REASONS   = "Unavailable For Legal Reasons";
const std::string HttpResponse::HTTP_REASON_INTERNAL_SERVER_ERROR           = "Internal Server Error";
const std::string HttpResponse::HTTP_REASON_NOT_IMPLEMENTED                 = "Not Implemented";
const std::string HttpResponse::HTTP_REASON_BAD_GATEWAY                     = "Bad Gateway";
const std::string HttpResponse::HTTP_REASON_SERVICE_UNAVAILABLE             = "Service Unavailable";
const std::string HttpResponse::HTTP_REASON_GATEWAY_TIMEOUT                 = "Gateway Time-Out";
const std::string HttpResponse::HTTP_REASON_VERSION_NOT_SUPPORTED           = "HTTP Version Not Supported";
const std::string HttpResponse::HTTP_REASON_VARIANT_ALSO_NEGOTIATES         = "Variant Also Negotiates";
const std::string HttpResponse::HTTP_REASON_INSUFFICIENT_STORAGE            = "Insufficient Storage";
const std::string HttpResponse::HTTP_REASON_LOOP_DETECTED                   = "Loop Detected";
const std::string HttpResponse::HTTP_REASON_NOT_EXTENDED                    = "Not Extended";
const std::string HttpResponse::HTTP_REASON_NETWORK_AUTHENTICATION_REQUIRED = "Network Authentication Required";
const std::string HttpResponse::HTTP_REASON_UNKNOWN                         = "???";
const std::string HttpResponse::DATE                                        = "Date";
const std::string HttpResponse::SET_COOKIE                                  = "Set-Cookie";
const std::string HttpResponse::AUTOINDEX_HTML                              = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><title></title></head><style>table {border: 1px solid #ccc;background-color: #f8f8f8;border-collapse: collapse;margin: 0;padding: 0;width: 100%;table-layout: fixed;text-align: left;}table td:last-child {border-bottom: 0;}</style><body></body></html>";


