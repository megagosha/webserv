//
// Created by Elayne Debi on 9/30/21.
//

/*
 * Normalize path
 * check for empty path. is it possible?
 * find location;
 * Method allowed for this location?
 * Append root
 * If request to folder
 * If autoindex on -> return autoindex page
 * If index specified -> append index to route
 * Construct response
 */


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
#include "Session.hpp"

class VirtualServer;

class HttpResponse;

class HttpRequest;

class Session;

class Socket
{
private:
	int _socket_fd;
	in_addr_t _ip;
	uint16_t _port;
	VirtualServer *_default_config;
	std::map<int, Session> _sessions;
	std::map<std::string, VirtualServer> _virtual_servers; //@todo change container
	Socket();

public:

	Socket(in_addr_t ip, uint16_t port, const VirtualServer &serv);

	Socket(const Socket &);

	~Socket();

	Socket &operator=(const Socket &);

	VirtualServer *getServerByHostHeader(const std::map<std::string, std::string> &headers);

	VirtualServer *getDefaultConfig(void) const;

	int getSocketFd() const;

	in_addr_t getIp() const;

	uint16_t getPort() const;

	const std::map<std::string, VirtualServer> &getVirtualServers() const;

	void appendVirtualServer(const VirtualServer &virtual_server);

	void removeSession(int fd);

	std::pair<int, Session *> acceptConnection(void);

	void clear();

	//@todo make sure virtual servers in each socket have unique server_name (only one empty server name per ip:port pair)
	//@todo validate that each virtual server has at least one location and server root
	class SocketException : public std::exception
	{
		const char *m_msg;
	public:
		explicit SocketException(const char *msg);

		~SocketException() throw();

		const char *what() const throw();
	};
};

#endif //SOCKET_HPP
