//
// Created by Elayne Debi on 9/30/21.
//

#include "Socket.hpp"

Socket::Socket()
{

}

Socket::Socket(in_addr_t ip, uint16_t port, const VirtualServer &serv) :

		_ip(ip), _port(port)
{
	appendVirtualServer(serv);
	sockaddr_in addr = {};
	int fd;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	_socket_fd = fd;
	std::cout << "socket created " << fd << std::endl;
	if (fd < 0)
		throw SocketException(std::strerror(errno));
	fcntl(fd, F_SETFL, O_NONBLOCK);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port); //port number;
	addr.sin_addr.s_addr = _ip;
	memset(addr.sin_zero, 0, 8);
	addr.sin_len = sizeof(addr);
	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	{
		close(fd);
		throw SocketException(std::strerror(errno));
	}
	if (listen(fd, MAX_AWAIT_CONN) == -1)
	{
		close(fd);
		throw SocketException(std::strerror(errno));
	}
}

Socket::Socket(const Socket &rhs) : _socket_fd(rhs._socket_fd),
									_ip(rhs._ip),
									_port(rhs._port),
									_virtual_servers(rhs._virtual_servers)
{
	_default_config = nullptr;
	for (std::map<std::string, VirtualServer>::iterator it = _virtual_servers.begin();
		 it != _virtual_servers.end(); ++it)
	{
		if (it->second == *rhs.getDefaultConfig())
		{
			_default_config = &it->second;
			break;
		}
	}
}

Socket::~Socket()
{
	for (std::map<int, Session>::iterator it = _sessions.begin(); it != _sessions.end(); ++it)
	{
		it->second.end();
	}
}

Socket &Socket::operator=(const Socket &rhs)
{
	if (this == &rhs)
		return (*this);
	_socket_fd = rhs.getSocketFd();
	_ip = rhs.getIp();
	_port = rhs.getPort();
	_virtual_servers = rhs.getVirtualServers();
	_default_config = nullptr;
	for (std::map<std::string, VirtualServer>::iterator it = _virtual_servers.begin();
		 it != _virtual_servers.end(); ++it)
	{
		if (it->second == *rhs.getDefaultConfig())
		{
			_default_config = &it->second;
			break;
		}
	}
	return (*this);
}

//Socket::Socket(in_addr_t ip,
//			   uint16_t port,
//			   int fd,
//			   VirtualServer &serv) :
//		_socket_fd(fd),
//		_ip(ip),
//		_port(port), _default_config(nullptr)
//{
//	appendVirtualServer(serv);
//};


std::pair<int, Session *> Socket::acceptConnection()
{
	int new_fd;
	struct sockaddr s_addr = {};
	socklen_t s_len;
	std::pair<std::map<int, Session>::iterator, bool> res;

	new_fd = accept(_socket_fd, &s_addr,
					&s_len); //@todo To ensure that accept() never blocks, the passed socket sockfd needs to have the O_NONBLOCK
	if (new_fd < 0)
		throw SocketException("Failed to open connection");
	res = _sessions.insert(std::make_pair(new_fd, Session(new_fd, this, s_addr)));
	return (std::make_pair(new_fd, &((res.first)->second)));
}


void Socket::removeSession(int fd)
{
	_sessions.erase(fd);
}

void Socket::appendVirtualServer(const VirtualServer &virtual_server)
{
	bool empty = false;
	if (_virtual_servers.empty())
		empty = true;
	std::pair<std::map<std::string, VirtualServer>::iterator, bool> res;
	res = _virtual_servers.insert(std::make_pair(virtual_server.getServerName(), virtual_server));
	if (!res.second)
		throw SocketException("Duplicate virtual server");
	if (empty)
		_default_config = &(res.first->second);

}

void Socket::clear()
{
	close(_socket_fd);
}

/*
 * @return
 * - nullptr on error
 * - default if empty
 * - config if found
 */
VirtualServer *Socket::getServerByHostHeader(const std::map<std::string, std::string> &headers)
{
	std::map<std::string, std::string>::const_iterator it = headers.find("Host");
	if (it == headers.end())
		return (nullptr);
	std::string host_value = it->second.substr(0, it->second.find(":")); //truncate port
	if (it->second.empty())
		return (_default_config);
	std::map<std::string, VirtualServer>::iterator ity = _virtual_servers.find(host_value);
	if (ity == _virtual_servers.end())
		return (_default_config);
	return (&ity->second);
}

VirtualServer *Socket::getDefaultConfig(void) const
{
	return _default_config;
}

int Socket::getSocketFd() const
{
	return _socket_fd;
}


in_addr_t Socket::getIp() const
{
	return _ip;
}


uint16_t Socket::getPort() const
{
	return _port;
}

const std::map<std::string, VirtualServer> &Socket::getVirtualServers() const
{
	return _virtual_servers;
}

////@todo make sure virtual servers in each socket have unique server_name (only one empty server name per ip:port pair)
////@todo validate that each virtual server has at least one location and server root

Socket::SocketException::SocketException(const char *msg) : m_msg(msg)
{}

Socket::SocketException::~SocketException() throw()
{};

const char *Socket::SocketException::what() const throw()
{
	std::cerr << "SocketError: ";
	return (m_msg);
}