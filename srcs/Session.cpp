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
    _request = rhs._request;
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

Session::Session(int socket_fd, Socket *server_socket, sockaddr addr, IManager *mng) :
        _fd(socket_fd), _server_socket(server_socket), _response(nullptr), _request(nullptr), _s_addr(addr),
        _keep_alive(false),
        _status(AWAIT_NEW_REQ), _connection_timeout(), _mng(mng) {
    std::time(&_connection_timeout);
    mng->subscribe(socket_fd, EVFILT_READ, this);
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
    std::string res(bytes, 0);
    try {
        Utils::recv(bytes, _fd, res); //@todo double copy! read directly into session buffer
        _buffer.insert(_buffer.end(), res.begin(), res.end());
    }
    catch (std::exception &e) {
        end();
    }
    size_t      pos = 0;
    if (_request == nullptr) {
        _request = new HttpRequest(getIpFromSock());
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
        if ((it != _request->getHeaderFields().end() && it->second == "close") || _request->getParsingError() == HttpResponse::HTTP_BAD_REQUEST)
            _keep_alive = false;
        else
            _keep_alive = true;
    }
    std::time(&_connection_timeout);
}

void Session::prepareResponse() {
    std::string   path;
//    VirtualServer *config = _server_socket->getServerByHostHeader(
//            _request->getHeaderFields());
//    std::cout << "res " << config->getServerName() << std::endl;
    //@todo null check
    if (_response != nullptr)
        _response->responsePrepare(_request, _mng);
//    _response = new HttpResponse(*this, config, _mng);
    if (_keep_alive) {
        _response->insertHeader("Connection", "Keep-Alive");
        _response->insertHeader("Keep-Alive", "timeout=5");
    }
    if (_response->getCgi() != nullptr && _response->getStatusCode() == HttpResponse::HTTP_OK) {
        _status = CGI_PROCESSING;
    } else
        _status = SENDING;
}

bool Session::writeCgi(size_t bytes, bool eof) {
    if (eof && _response->getCgi()->getPos() < _request->getBody().size()) {
        std::cout << "READER DISCONECTED" << std::endl;
        return (true);
    } else if (_response->writeToCgi(_request, bytes))
        return (true);
    return (false);
}


bool Session::readCgi(size_t bytes, bool eof) {
    if (_response->readCgi(bytes, eof)) {

//        if (!_response->getCgi()->cgiEnd())
//            std::cout << "cgi fast quit" << std::endl;
//            _response->setError(HttpResponse::HTTP_INTERNAL_SERVER_ERROR, nullptr);
        std::cout << "READY TO SEND CGI RESPONSE" << std::endl;
        _status = SENDING;
        return (true);
    } else
        return (false);

}

bool Session::shouldClose() {

    if (!isKeepAlive() && _status != AWAIT_NEW_REQ)
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
//        _response->sendResponse(_fd, nullptr);
    }
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

const std::string &Session::getBuffer() const {
    return _buffer;
}

std::string &Session::getBuffer() {
    return _buffer;
}

void Session::clearBuffer(void) {
    _buffer.clear();
}

void Session::processResponse(size_t bytes) {
    if (_response->sendResponse(_fd, _request, bytes) == 1) {
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
}

void Session::processCurrentStatus(short status) {
    if (status == AWAIT_NEW_REQ)
        _mng->subscribe(_fd, EVFILT_READ, this);
    if (status == CGI_PROCESSING) {
        if (!_request->getBody().empty())
            _mng->subscribe(_response->getCgi()->getRequestPipe(), EVFILT_WRITE, this);
        _mng->subscribe(_response->getCgi()->getResponsePipe(), EVFILT_READ, this);
        _mng->subscribe(_response->getCgi()->getCgiPid(), EVFILT_PROC, this);
        return;
    }
    if (status == SENDING) {
        _mng->subscribe(_fd, EVFILT_WRITE, this);
        return;
    }
    if (status == CLOSING) {
        _mng->unsubscribe(_fd, EVFILT_READ);
        end();
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

    if (bytes_available == 0 && eof && fd == _fd)
        end();
    else if (_status == AWAIT_NEW_REQ && filter == EVFILT_READ && fd == _fd) {
        parseRequest(bytes_available);
        if (_request->getMethod() == "HEAD")
            std::cout << "Yo" << std::endl;
        if (_request->isReady())
            prepareResponse();
    } else if (_status == CGI_PROCESSING && filter == EVFILT_WRITE && fd == _response->getCgi()->getRequestPipe()) {
        writeCgi(bytes_available, eof);
    } else if (_status == CGI_PROCESSING && filter == EVFILT_READ && fd == _response->getCgi()->getResponsePipe()) {
        readCgi(bytes_available, eof);
    } else if (_status == SENDING && fd == _fd) {
        processResponse(bytes_available);
    } else if (_fd == fd && eof) {
        _status = CLOSING;
    }
    if (filter == EVFILT_PROC && (flags & NOTE_EXIT) && _response->getCgi() != nullptr) {
        delete _response->getCgi();
        _response->setCgi(nullptr);
    }

    //@todo create two functions sub and unsub. split everything
    if (prev_status != _status) {
        processPreviousStatus(prev_status);
        processCurrentStatus(_status);
    }
}

IManager *Session::getMng() const {
    return _mng;
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
