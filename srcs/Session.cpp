//
// Created by George Tevosov on 17.10.2021.
//

#include "Session.hpp"

Session::Session()
		: _fd(), _server_socket(), _response(nullptr), _keep_alive(false), _response_sent(), _connection_timeout(),
		  _receive_timeout(),
		  _last_update()
{

}

Session::Session(const Session &rhs) : _fd(rhs._fd), _server_socket(rhs._server_socket), _keep_alive(rhs._keep_alive),
									   _response_sent(rhs._response_sent),
									   _connection_timeout(rhs._connection_timeout),
									   _receive_timeout(rhs._receive_timeout), _last_update(rhs._last_update)
{
	setResponse(rhs._response);
}

Session &Session::operator=(const Session &rhs)
{
	if (this == &rhs)
		return (*this);
	_fd                 = rhs._fd;
	_server_socket      = rhs._server_socket;
	_keep_alive         = rhs._keep_alive;
	_response_sent      = rhs._response_sent;
	_connection_timeout = rhs._connection_timeout;
	_receive_timeout    = rhs._receive_timeout;
	_last_update        = rhs._last_update;
	setResponse(rhs._response);

	return (*this);
}

Session::Session(Socket *sock)
		: _fd(), _server_socket(sock), _response(nullptr), _keep_alive(), _response_sent(), _connection_timeout(),
		  _receive_timeout(),
		  _last_update()
{
	time(&_last_update);
}

Session::Session(int socket_fd, Socket *server_socket, sockaddr addr) :
		_fd(socket_fd), _server_socket(server_socket), _response(nullptr), _s_addr(addr), _keep_alive(false)
{
	time(&_last_update);
}

Session::~Session(void)
{
	if (_response != nullptr)
		delete _response;
}

int Session::getFd() const
{
	return _fd;
}

void Session::setFd(int fd)
{
	_fd = fd;
}

Socket *Session::getServerSocket() const
{
	return _server_socket;
}

void Session::setServerSocket(Socket *serverSocket)
{
	_server_socket = serverSocket;
}

const HttpResponse *Session::getResponse() const
{
	return _response;
}

void Session::setResponse(const HttpResponse *response)
{

	if (_response != nullptr)
	{
//		delete _response; @todo fix memory err
		_response = nullptr;
	}
	if (response == nullptr)
	{
		_response = nullptr;
		return;
	}
	_response = new HttpResponse(*response);
}

const HttpRequest &Session::getRequest() const
{
	return _request;
}

void Session::setRequest(const HttpRequest &request)
{
	_request = request;
}

bool Session::isKeepAlive() const
{
	return _keep_alive;
}

void Session::setKeepAlive(bool keepAlive)
{
	_keep_alive = keepAlive;
}

bool Session::isResponseSent() const
{
	return _response_sent;
}

void Session::setResponseSent(bool responseSent)
{
	_response_sent = responseSent;
}

time_t Session::getConnectionTimeout() const
{
	return _connection_timeout;
}

void Session::setConnectionTimeout(time_t connectionTimeout)
{
	_connection_timeout = connectionTimeout;
}

time_t Session::getReceiveTimeout() const
{
	return _receive_timeout;
}

void Session::setReceiveTimeout(time_t receiveTimeout)
{
	_receive_timeout = receiveTimeout;
}

time_t Session::getLastUpdate() const
{
	return _last_update;
}

void Session::setLastUpdate(time_t lastUpdate)
{
	_last_update = lastUpdate;
}

void Session::parseRequest(long bytes)
{
	std::string res(bytes, 0);
	Utils::recv(bytes, _fd, res);
	_request = HttpRequest(res, Utils::ClientIpFromSock(&_s_addr));
}

void Session::prepareResponse()
{
	std::string   path;
	VirtualServer *config = _server_socket->getServerByHostHeader(
			_request.getHeaderFields());
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

bool Session::ifEnd(void)
{
	if (!isKeepAlive())
	{
		end();
		return (true);
	} else
		return (false);
}

void Session::end(void)
{
	close(_fd);
	_server_socket->removeSession(_fd);
}

void Session::send()
{
	_response->sendResponse(_fd, &_request);
	if (!isKeepAlive())
		end();
}

