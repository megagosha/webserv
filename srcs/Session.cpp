//
// Created by George Tevosov on 17.10.2021.
//

#include "Session.hpp"

Session::Session()
        : _fd(), _server_socket(), _response(nullptr), _request(nullptr), _s_addr(), _keep_alive(false),
          _status(UNUSED), _mng(nullptr) {
}


Session::Session(int socket_fd, Socket *server_socket, sockaddr addr, IManager *mng) :
        _fd(socket_fd), _server_socket(server_socket), _response(nullptr), _request(nullptr), _s_addr(addr),
        _keep_alive(false),
        _status(UNUSED), _connection_timeout(), _mng(mng) {
    std::time(&_connection_timeout);
//    std::cout << "Session created:" << std::endl << "Client ip: " << getClientIp() << " port: " << getClientPort()
//              << std::endl;
    mng->subscribe(socket_fd, EVFILT_READ, this);
}

Session::Session(const Session &rhs) : _fd(rhs._fd), _server_socket(rhs._server_socket), _s_addr(),
                                       _keep_alive(rhs._keep_alive),
                                       _status(rhs._status),
                                       _connection_timeout(rhs._connection_timeout), _mng(rhs._mng) {
    setResponse(rhs._response);
    _request = rhs._request;
}

Session::~Session(void) {
    if (_response != nullptr)
        delete _response;
    if (_request != nullptr)
        delete _request;
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
    _request = rhs._request;
    return (*this);
}

std::string Session::getClientPort() const {
    struct sockaddr_in *sin = (struct sockaddr_in *) &_s_addr;
    uint16_t           port;

    port = htons (sin->sin_port);
    return (std::to_string(port));
}

std::string Session::getClientIp() const {
    struct sockaddr_in *sin = (struct sockaddr_in *) &_s_addr;
    char               ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &(sin->sin_addr), ip, INET_ADDRSTRLEN);
    return (ip);
}

void Session::setResponse(HttpResponse *response) {

    if (_response != nullptr) {
//		delete _response; @todo fix memory err
        _response = nullptr;
        return;
    }
    _response = response;
}

HttpRequest *Session::getRequest() const {
    return _request;
}


bool Session::isKeepAlive() const {
    return _keep_alive;
}

void Session::setKeepAlive(bool keepAlive) {
    _keep_alive = keepAlive;
}

//time_t Session::getConnectionTimeout() const {
//    return _connection_timeout;
//}
//
//void Session::setConnectionTimeout() {
//    time(&_connection_timeout);
//}

std::string Session::getIpFromSock() {
    return (Utils::ClientIpFromSock(&_s_addr));
}

void Session::parseRequest(size_t bytes) {
    std::string res(bytes, 0);
    size_t      pos = 0;

    try {
        Utils::recv(bytes, _fd, res); //@todo double copy! read directly into session buffer
        _buffer.insert(_buffer.end(), res.begin(), res.end());
    }
    catch (std::exception &e) {
        return;
    }
    if (_request == nullptr) {
        _request = new HttpRequest();
    }
    if (_request->getRequestUri().empty())
        _request->parseRequestMessage(this, pos, _server_socket);
    else
        _request->appendBody(this, pos);
    if (_request->isReady()) {
        if (_response == nullptr)
            _response = new HttpResponse(static_cast<HttpResponse::HTTPStatus>(_request->getParsingError()));
        _buffer.clear();
    }
    if (!_request->getRequestUri().empty()) {
        std::map<std::string, std::string>::const_iterator it;
        it              = _request->getHeaderFields().find("Connection");
        if ((it != _request->getHeaderFields().end() && it->second == "close") ||
            _request->getParsingError() == HttpResponse::HTTP_BAD_REQUEST)
            _keep_alive = false;
        else
            _keep_alive = true;
    }
    std::time(&_connection_timeout);
}

void Session::prepareResponse() {
    std::string path;

    //@todo null check
    if (_response != nullptr)
        _response->responsePrepare(_request, _mng, getIpFromSock());
    if (_keep_alive) {
        _response->insertHeader("Connection", "Keep-Alive");
        _response->insertHeader("Keep-Alive", "timeout=" + std::to_string(HTTP_DEFAULT_TIMEOUT));
    }
    if (_response->getCgi() != nullptr && _response->getStatusCode() == HttpResponse::HTTP_OK) {
        _status = CGI_PROCESSING;
    } else
        _status = SENDING;
}

bool Session::writeCgi(size_t bytes, bool eof) {
    if (eof && _response->getCgi()->getPos() < _request->getBody().size()) {
        return (true);
    } else if (_response->writeToCgi(_request, bytes))
        return (true);
    return (false);
}

bool Session::readCgi(size_t bytes, bool eof) {
    if (_response->readCgi(bytes, eof)) {
        _status = SENDING;
        return (true);
    } else
        return (false);

}

bool Session::shouldClose() {

    if ((!isKeepAlive() && _status == AWAIT_NEW_REQ) || _status == CLOSING)
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
    processPreviousStatus(_status);
    close(_fd);
    _server_socket->removeSession(_fd);
}

short Session::getStatus() const {
    return _status;
}

void Session::setStatus(short status) {
    _status = status;
}

//const std::string &Session::getBuffer() const {
//    return _buffer;
//}

std::string &Session::getBuffer() {
    return _buffer;
}

void Session::clearBuffer(void) {
    _buffer.clear();
}

void Session::processResponse(size_t bytes, bool eof) {
    if (eof || _response->sendResponse(_fd, _request, bytes) == 1) {
        if (!_keep_alive)
            _status = CLOSING;
        else
            _status = AWAIT_NEW_REQ;
        if (_request != nullptr) {
            delete _request;
            _request = nullptr;
        }
        if (_response != nullptr) {
            delete _response;
            _response = nullptr;
        }
    }
    std::time(&_connection_timeout);
}

void Session::processCurrentStatus(short status) {
    if (status == AWAIT_NEW_REQ)
        _mng->subscribe(_fd, EVFILT_READ, this);
    if (status == CGI_PROCESSING) {
        if (!_request->getBody().empty())
            _mng->subscribe(_response->getCgi()->getRequestPipe(), EVFILT_WRITE, this);
        _mng->subscribe(_response->getCgi()->getResponsePipe(), EVFILT_READ, this);
        return;
    }
    if (status == SENDING) {
        _mng->subscribe(_fd, EVFILT_WRITE, this);
        return;
    }
    if (status == CLOSING) {
        _mng->unsubscribe(_fd, EVFILT_READ);
    }
}

void Session::processPreviousStatus(short prev_status) {
    if (prev_status == AWAIT_NEW_REQ)
        _mng->unsubscribe(_fd, EVFILT_READ);
    else if (prev_status == CGI_PROCESSING) {
        if (!_request->getBody().empty())
            _mng->unsubscribe(_response->getCgi()->getRequestPipe(), EVFILT_WRITE);
        _mng->unsubscribe(_response->getCgi()->getResponsePipe(), EVFILT_READ);
        return;
    } else if (prev_status == SENDING) {
        _mng->unsubscribe(_fd, EVFILT_WRITE);
        return;
    }
}

void Session::processEvent(int fd, size_t bytes_available, int16_t filter, __unused uint32_t flags, bool eof,
                           __unused Server *serv) {
    //@todo
    short prev_status = _status;

    // close if eof on write socket or eof on read while request was not parsed completely;
    if (_fd == fd && eof &&
        (filter == EVFILT_WRITE ||
         (filter == EVFILT_READ && (_request == nullptr || !_request->isReady()))))
        end();
    else if ((_status == AWAIT_NEW_REQ || _status == UNUSED) && filter == EVFILT_READ && fd == _fd &&
             bytes_available > 0) {
        parseRequest(bytes_available);
        if (_request->isReady())
            prepareResponse();
    } else if (_status == CGI_PROCESSING && filter == EVFILT_WRITE && fd == _response->getCgi()->getRequestPipe()) {
        writeCgi(bytes_available, eof);
    } else if (_status == CGI_PROCESSING && filter == EVFILT_READ && fd == _response->getCgi()->getResponsePipe()) {
        readCgi(bytes_available, eof);
    } else if (_status == SENDING && filter == EVFILT_WRITE && fd == _fd) {
        processResponse(bytes_available, eof);
    }
    if (prev_status != _status) {
        processPreviousStatus(prev_status);
        processCurrentStatus(_status);
    }
}

Session::SessionException::SessionException(const std::string &msg) {
    std::cout << msg << std::endl;
    throw;
}

Session::SessionException::~SessionException() throw() {

}

const char *Session::SessionException::what() const throw() {
    return exception::what();
}
