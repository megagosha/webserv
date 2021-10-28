//
// Created by George Tevosov on 17.10.2021.
//

#include "Session.hpp"

Session::Session()
        : _fd(), _server_socket(), _response(nullptr), _request(nullptr), _s_addr(), _keep_alive(false),
          _status(UNUSED) {
}

Session::Session(const Session &rhs) : _fd(rhs._fd), _server_socket(rhs._server_socket), _s_addr(),
                                       _keep_alive(rhs._keep_alive),
                                       _status(rhs._status),
                                       _connection_timeout(rhs._connection_timeout) {
    setResponse(rhs._response);
    setRequest(rhs._request);
}

Session &Session::operator=(const Session &rhs) {
    if (this == &rhs)
        return (*this);
    _fd                 = rhs._fd;
    _server_socket      = rhs._server_socket;
    _keep_alive         = rhs._keep_alive;
    _status             = rhs._status;
    _connection_timeout = rhs._connection_timeout;
    setResponse(rhs._response);
    setRequest(rhs._request);
    return (*this);
}

Session::Session(int socket_fd, Socket *server_socket, sockaddr addr) :
        _fd(socket_fd), _server_socket(server_socket), _response(nullptr), _request(nullptr), _s_addr(addr),
        _keep_alive(false),
        _status(UNUSED), _connection_timeout() {
    std::time(&_connection_timeout);
}

Session::~Session(void) {
    if (_response != nullptr)
        delete _response;
    if (_request != nullptr)
        delete _request;
}

int Session::getFd() const {
    return _fd;
}

void Session::setFd(int fd) {
    _fd = fd;
}

Socket *Session::getServerSocket() const {
    return _server_socket;
}

void Session::setServerSocket(Socket *serverSocket) {
    _server_socket = serverSocket;
}

const HttpResponse *Session::getResponse() const {
    return _response;
}

void Session::setResponse(const HttpResponse *response) {

    if (response == nullptr) {
//		delete _response; @todo fix memory err
        _response = nullptr;
        return;
    }
    _response = new HttpResponse(*response);
}

HttpRequest *Session::getRequest() const {
    return _request;
}

void Session::setRequest(HttpRequest *request) {
//    if (_request != nullptr) {
//        //delete _request; // @todo fix memory err
//    }
    if (request == nullptr) {
        _request = nullptr;
        return;
    }
    _request = new HttpRequest(*request);
}

bool Session::isKeepAlive() const {
    return _keep_alive;
}

void Session::setKeepAlive(bool keepAlive) {
    _keep_alive = keepAlive;
}

time_t Session::getConnectionTimeout() const {
    return _connection_timeout;
}

void Session::setConnectionTimeout() {
    time(&_connection_timeout);
}

std::string Session::getIpFromSock() {
    return (Utils::ClientIpFromSock(&_s_addr));
}

void Session::parseRequest(long bytes) {
//	if (bytes > Socket::DEF)
//		bytes = DEFAULT_MAX_SIZE;
    std::string res(bytes, 0); //@todo think about file size limit;
    Utils::recv(bytes, _fd, res);
    if (_request == nullptr) {
        _request = new HttpRequest(res, getIpFromSock(), bytes, _server_socket);
    }
    else
        _request->appendBody(res, bytes);
    std::cout << "parsed request size " << _request->getBody().size() << std::endl;
    std::cout << "chunked " << _request->isChunked() << std::endl;
    std::map<std::string, std::string>::const_iterator it;
    it              = _request->getHeaderFields().find("Connection");
    if (it != _request->getHeaderFields().end() && it->second == "keep-alive")
        _keep_alive = true;
    else
        _keep_alive = false;
}

void Session::prepareResponse() {
    std::string   path;
    VirtualServer *config = _server_socket->getServerByHostHeader(
            _request->getHeaderFields());
    //@todo null check
    _response = new HttpResponse(*this, config);
    if (_keep_alive)
    {
    	_response->insertHeader("Connection", "Keep-Alive");
    	_response->insertHeader("Keep-Alive", "timeout=5");

    }
    _status   = READY_TO_SEND;
//	if (config == nullptr)
//	{
//		_response = HttpResponse(400, *config);
//		return;
//	}  //@todo create  ErrorResponseClass;
//	loc = config->getLocationFromRequest(_request);

//	if (loc == nullptr)
//	{
//		_response = HttpResponse(404, *config);
//		return;
//	}
//	if (!loc->methodAllowed(_request.getMethod()))
//	{
//		_response = HttpResponse(405, *config);
//		return;
//	}
//	path = _request.getNormalizedPath();
//	path = path.replace(0, 1, loc->getRoot());
//	if (path == )

}

bool Session::shouldClose() {

    if (!isKeepAlive() && _status == AWAIT_NEW_REQ)
        return (true);
    time_t cur_time;
    std::time(&cur_time);
    if (std::difftime(cur_time, _connection_timeout) > HTTP_DEFAULT_TIMEOUT) {
        _status = TIMEOUT;
        return (true);
    }
    return (false);
}

void Session::end() {
    std::cout << "Session closed" << std::endl;
    if (_status == TIMEOUT) {
        _response = new HttpResponse(HttpResponse::HTTP_REQUEST_TIMEOUT, _server_socket->getDefaultConfig());
        _response->sendResponse(_fd, nullptr);
    }
    close(_fd);
    _server_socket->removeSession(_fd);
}

void Session::send() {
    _response->sendResponse(_fd, _request);
    _status = AWAIT_NEW_REQ;
    time(&_connection_timeout);
    if (_response != nullptr) {
        delete _response;
        _response = nullptr;
    }
    if (_request != nullptr) {
        delete _request;
        _request = nullptr;
    }
}

short Session::getStatus() const {
    return _status;
}

void Session::setStatus(short status) {
    _status = status;
}

