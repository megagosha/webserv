//
// Created by Elayne Debi on 9/9/21.
//

#ifndef SERVER_HPP
#define SERVER_HPP

#include "Socket.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "KqueueEvents.hpp"
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include "utils.hpp"
#include <list>
#include "VirtualServer.hpp"

#define MAX_AWAIT_CONN 100
#define MAX_KQUEUE_EV 100

//@todo Coplien form should be implemented
class Server
{
	typedef std::map<int, Socket>::iterator iter;

private:
	std::map<int, Socket> _sockets; // <socket fd, socket obj>
	std::map<int, iter> _connections; // <connection fd, socket iterator>
//	std::string config_res;
	std::list<std::string> _tok_list;
	std::set<std::string> _config_keys;
//    std::map<int, VirtualServer> servers;
	std::map<int, HttpResponse> _pending_response;
	//pair connection fd with server fd
//    std::map<int, int> connections;
	KqueueEvents _kq;

	Server();

public:

	Server &operator=(const Server &rhs);

	Server(const Server &rhs);

	//@todo create destructor
	~Server();

	Server(const std::string &config_file);

	void process_requests(std::pair<int, struct kevent *> &updates);

//	HttpResponse generateResponse(HttpRequest &request);

	void process_response(std::pair<int, struct kevent *> &updates);

	_Noreturn void run(void);

	bool validate(const VirtualServer &server);

	void apply(VirtualServer &serv);

	void init_keys();

	class ServerException : public std::exception
	{
		const std::string m_msg;
	public:
		ServerException(const std::string &msg);

		~ServerException() throw();

		const char *what() const throw();
	};
};

#endif //UNTITLED_SERVER_HPP
