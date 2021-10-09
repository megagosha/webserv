//
// Created by Elayne Debi on 9/30/21.
//


#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "VirtualServer.hpp"
#include <map>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class VirtualServer;
class HttpResponse;
class HttpRequest;
class Socket
{
private:
	int _socket_fd;
    in_addr_t _ip;
	uint16_t _port;
	VirtualServer *_default_config;
	std::map<std::string, VirtualServer> _virtual_servers; //@todo change container
	Socket();
public:

	Socket &operator=(const Socket &);

	Socket(const Socket &);

	~Socket();

	VirtualServer *getDefaultConfig(void) const;

	int getSocketFd() const;

	void setSocketFd(int socketFd);

    in_addr_t getIp() const;

	void setIp(in_addr_t ip);

	uint16_t getPort() const;

	void setPort(uint16_t port);

	const std::map<std::string, VirtualServer> &getVirtualServers() const;

	void appendVirtualServer(const VirtualServer &virtual_server);

	Socket(in_addr_t ip, uint16_t port, int fd, VirtualServer &sock);

	Socket(in_addr_t ip, uint16_t port, const VirtualServer &serv);

	void clear();

	//@todo make sure virtual servers in each socket have unique server_name (only one empty server name per ip:port pair)
	//@todo validate that each virtual server has at least one location and server root
	HttpResponse generate(const HttpRequest &request);

public:
	class SocketException : public std::exception
	{
		const char *m_msg;
	public:
		explicit SocketException(const char *msg);

		~SocketException() throw();

		const char *what() const throw();
	};
};

#endif //WEBSERV_SOCKET_HPP
