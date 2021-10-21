//
// Created by George Tevosov on 17.10.2021.
//

#include "Session.hpp"

Session::Session()
        : _fd(), _server_socket(), _response(nullptr), _request(nullptr), _s_addr(), _keep_alive(false),
          _response_sent(),
          _connection_timeout(),
          _last_response_sent() {
}

Session::Session(const Session &rhs) : _fd(rhs._fd), _server_socket(rhs._server_socket), _s_addr(),
                                       _keep_alive(rhs._keep_alive),
                                       _response_sent(rhs._response_sent),
                                       _connection_timeout(rhs._connection_timeout),
                                       _last_response_sent(rhs._last_response_sent) {
    setResponse(rhs._response);
    setRequest(rhs._request);
}

Session &Session::operator=(const Session &rhs) {
    if (this == &rhs)
        return (*this);
    _fd = rhs._fd;
    _server_socket = rhs._server_socket;
    _keep_alive = rhs._keep_alive;
    _response_sent = rhs._response_sent;
    _connection_timeout = rhs._connection_timeout;
    _last_response_sent = rhs._last_response_sent;
    setResponse(rhs._response);
    setRequest(rhs._request);
    return (*this);
}

Session::Session(Socket *sock)
        : _fd(), _server_socket(sock), _response(nullptr), _s_addr(), _keep_alive(), _response_sent(),
          _connection_timeout(),
          _last_response_sent() {
}

Session::Session(int socket_fd, Socket *server_socket, sockaddr addr) :
        _fd(socket_fd), _server_socket(server_socket), _response(nullptr), _request(nullptr), _s_addr(addr),
        _keep_alive(false),
        _response_sent(), _connection_timeout(), _last_response_sent(0) {
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
        _response = nullptr;return;
    }
    _response = new HttpResponse(*response);
}

const HttpRequest *Session::getRequest() const {
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

bool Session::isResponseSent() const {
    return _response_sent;
}

void Session::setResponseSent(bool responseSent) {
    _response_sent = responseSent;
}

time_t Session::getConnectionTimeout() const {
    return _connection_timeout;
}

void Session::setConnectionTimeout() {
    time(&_connection_timeout);
}

void Session::parseRequest(long bytes) {
    std::string res(bytes, 0);
    Utils::recv(bytes, _fd, res);
    if (_request == nullptr)
        _request = new HttpRequest(res, Utils::ClientIpFromSock(&_s_addr), bytes);
    else
        _request->appendBody(res, bytes);
    std::map<std::string, std::string>::const_iterator it;
    it = _request->getHeaderFields().find("Connection");
    if (it != _request->getHeaderFields().end() && it->second == "keep-alive")
        _keep_alive = true;
    else
        _keep_alive = false;
}

void Session::prepareResponse() {
    std::string path;
    VirtualServer *config = _server_socket->getServerByHostHeader(
            _request->getHeaderFields());
    _response = new HttpResponse(*this, config);
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
    if (!isKeepAlive()) {
        end();
        return (true);
    } else {
        time_t cur_time;
        std::time(&cur_time);
        if (std::difftime(_last_response_sent, cur_time) > 5) {
            return (true);
        }
    }
    return (false);
}

void Session::end() {
    close(_fd);
    _server_socket->removeSession(_fd);
}

void Session::send() {
    _response->sendResponse(_fd, _request);
    if (!isKeepAlive())
        end();
}

time_t Session::getLastResposnseSent() const {
    return (_last_response_sent);
}

void Session::setLastUpdate() {
    std::time(&_last_response_sent);
}

