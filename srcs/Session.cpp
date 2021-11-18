//
// Created by George Tevosov on 17.10.2021.
//

#include "Session.hpp"

Session::Session()
        : _fd(), _server_socket(), _response(nullptr), _request(nullptr), _s_addr(), _keep_alive(false),
          _status(AWAIT_NEW_REQ), _mng(nullptr) {
}

Session::Session(const Session &rhs) : _fd(rhs._fd), _server_socket(rhs._server_socket), _s_addr(),
                                       _keep_alive(rhs._keep_alive),
                                       _status(rhs._status),
                                       _connection_timeout(rhs._connection_timeout), _mng(rhs._mng) {
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
    _mng                = rhs._mng;
    setResponse(rhs._response);
    setRequest(rhs._request);
    return (*this);
}

Session::Session(int socket_fd, Socket *server_socket, sockaddr addr, IManager *mng) :
        _fd(socket_fd), _server_socket(server_socket), _response(nullptr), _request(nullptr), _s_addr(addr),
        _keep_alive(false),
        _status(AWAIT_NEW_REQ), _connection_timeout(), _mng(mng) {
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

//@todo setsockopt set min and max buffer size for send and receive
void Session::parseRequest(size_t bytes) {
//	if (bytes > Socket::DEF)
//		bytes = DEFAULT_MAX_SIZE;
    std::string res(bytes, 0);
    try {
        Utils::recv(bytes, _fd, res); //@todo double copy! read directly into session buffer
        _buffer.insert(_buffer.end(), res.begin(), res.end());
    }
    catch (std::exception &e) {
        end();
    }
//    std::cout << "printing buffer size" << std::endl;
//    std::cout << _buffer.size();
//    std::cout << "|buffer end" << std::endl;
    size_t      pos = 0;
    if (_request == nullptr)
        _request = new HttpRequest(this, getIpFromSock(), bytes);
    else if (_request->getRequestUri().empty())
        _request->parseRequestMessage(this, pos);
    else
        _request->appendBody(this, pos);
    if (_request->isReady()) {
        _buffer.clear();
    }
//    if (_request->getRequestUri() == "/directory")
//    	std::cout << "x" << std::endl;
//    std::cout << "parsed request size " << _request->getBody().size() << std::endl;
//    std::cout <<"method " << _request->getMethod() <<  " chunked " << _request->isChunked() << std::endl;
    if (!_request->getRequestUri().empty()) {
        std::map<std::string, std::string>::const_iterator it;
        it              = _request->getHeaderFields().find("Connection");
        if (it != _request->getHeaderFields().end() && it->second == "close")
            _keep_alive = false;
        else
            _keep_alive = true;
    }
}

void Session::prepareResponse() {
    std::string   path;
    VirtualServer *config = _server_socket->getServerByHostHeader(
            _request->getHeaderFields());
    std::cout << "res " << config->getServerName() << std::endl;
    //@todo null check
    _response = new HttpResponse(*this, config);
    if (_keep_alive) {
        _response->insertHeader("Connection", "Keep-Alive");
        _response->insertHeader("Keep-Alive", "timeout=5");
    }
    if (_response->getCgi() != nullptr && !_request->getBody().empty()) {
        _status = PIPE_TO_CGI;
    } else if (_response->getCgi() != nullptr) {
        _status = READ_FROM_CGI;
    } else {
        _status = SENDING;
    }
}

bool Session::writeCgi(size_t bytes, bool eof) {
    if (eof && _response->getCgi()->getPos() < _request->getBody().size()) {
        std::cout << "READER DISCONECTED" << std::endl;
        _status = READ_FROM_CGI;
    } else if (_response->writeToCgi(_request, bytes))
        _status = READ_FROM_CGI;
    else if (_response->getStatusCode() == HttpResponse::HTTP_INTERNAL_SERVER_ERROR)
        _status = SENDING;
    else return (false);

    if (_status == READ_FROM_CGI)
        _mng->subscribe(_response->getCgi()->getResponsePipe(), EVFILT_READ, this);
    else
        _mng->subscribe(_fd, EVFILT_WRITE, this);
    return (true);
}

bool Session::readCgi(size_t bytes, bool eof) {
    if (_response->readCgi(bytes, eof)) {
        std::cout << "READY TO SEND CGI RESPONSE" << std::endl;
        _status = SENDING;

        return (true);
    } else
        return (false);

}

//void Session::cgiClose()
//{
//
//}

bool Session::shouldClose() {

    if (!isKeepAlive() && _status == AWAIT_NEW_REQ)
        return (true);
    time_t cur_time;
    std::time(&cur_time);
//    if (std::difftime(cur_time, _connection_timeout) > HTTP_DEFAULT_TIMEOUT) {
//        _status = TIMEOUT;
//        return (true);
//    }
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

const std::string &Session::getBuffer() const {
    return _buffer;
}

std::string &Session::getBuffer() {
    return _buffer;
}

void Session::clearBuffer(void) {
    _buffer.clear();
}

void Session::processResponse(size_t bytes)
{
    if (_response->sendResponse(_fd, _request, bytes) == 1)
    {
        _status = AWAIT_NEW_REQ;
        if (_request != nullptr)
        {
            delete _request;
            _request = nullptr;
        }
        if (_response != nullptr)
        {
            delete _response;
            _response = nullptr;
        }
    }
}
void Session::processEvent(int fd, size_t bytes_available, int16_t filter, bool eof, Server *serv) {
    //@todo
    short prev_status = _status;
    do {
        if (_status == AWAIT_NEW_REQ && filter == EVFILT_READ) {
            parseRequest(bytes_available);
            if (_request->isReady())
                prepareResponse();
            break;
        }
        if (_status == PIPE_TO_CGI && filter == EVFILT_WRITE && fd == _response->getCgi()->getRequestPipe()) {
            writeCgi(bytes_available, eof);
            break;
        }
        if (_status == READ_FROM_CGI && filter == EVFILT_READ && fd == _response->getCgi()->getResponsePipe()) {
            readCgi(bytes_available, eof);
            break;
        }
        if (_status == SENDING) {

        }

    } while (false);

    //@todo create two functions sub and unsub. split everything

    if (_status == AWAIT_NEW_REQ)
            _mng->subscribe(_fd, EVFILT_READ, this);
    if (prev_status != _status) {
        if (prev_status == PIPE_TO_CGI)
            _mng->unsubscribe(_response->getCgi()->getRequestPipe(), EVFILT_WRITE, this);
        if (_status == PIPE_TO_CGI)
            _mng->subscribe(_response->getCgi()->getRequestPipe(), EVFILT_WRITE, this);
        if (_status == READ_FROM_CGI)
            _mng->subscribe(_response->getCgi()->getResponsePipe(), EVFILT_READ, this);
        if (prev_status == READ_FROM_CGI)
            _mng->unsubscribe(_response->getCgi()->getResponsePipe(), EVFILT_READ, this);
        if (_status == SENDING)
            _mng->subscribe(_fd, EVFILT_WRITE, this);
        if (prev_status == SENDING)
            _mng->unsubscribe(_fd, EVFILT_WRITE, this);
    }
}

