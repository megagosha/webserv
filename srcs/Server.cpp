//
// Created by Elayne Debi on 9/9/21.
//
#include "Server.hpp"


Server::ServerException::ServerException(const std::string &msg) : m_msg(msg)
{}

Server::ServerException::~ServerException() throw()
{};

const char *Server::ServerException::what() const throw()
{
	std::cerr << "ServerError: ";
	return (std::exception::what());
}


//@todo create destructor
Server::~Server()
{
	//close all connections.
	for (std::map<int, iter>::iterator it = _connections.begin(); it != _connections.end(); ++it)
	{
		close(it->first);
	}
	for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it)
	{
		it->second.clear();
	}
}

Server::Server(const std::string &config_file) : _kq(MAX_KQUEUE_EV)
{
	signal(SIGPIPE, SIG_IGN);

	Utils::tokenizeFileStream(config_file, _tok_list);
	std::list<std::string>::iterator end = _tok_list.end();
	std::list<std::string>::iterator it = _tok_list.begin();
	for (; it != end; ++it) // loop for servers
	{
		if (*it != "server" || *(++it) != "{")
			throw Server::ServerException("Error while parsing config file");
		VirtualServer serv;
		++it;
		std::list<std::string>::iterator check;
		while (it != end && *it != "}") //loop inside server block
		{
			if (it != end && *it == "listen")
			{
				serv.setHost(it, end);
				continue;
			}
			if (it != end && *it == "port")
			{
				serv.setPort(it, end);
				continue;
			}
			if (it != end && *it == "client_max_body_size")
			{
				serv.setBodySize(it, end);
				continue;
			}
			if (it != end && *it == "error_page")
			{
				serv.setErrorPage(it, end);
				continue;
			}
			if (it != end && *it == "location")
			{
				serv.setLocation(it, end);
				continue;
			}
			++it;
			//@todo THROW ERROR if nothing else worked
		}
		MimeType("/Users/megagosha/42/webserv/mime.conf");
		validate(serv);
		apply(serv);

	}
	run();
}

void Server::process_requests(std::pair<int, struct kevent *> &updates)
{
	int i = 0;
	int new_fd = 0;
	std::map<int, Socket>::iterator it;
	std::map<int, iter>::iterator ity;
	struct sockaddr s_addr = {};
	socklen_t s_len;

	std::cout << "Kqueue update size " << updates.first << std::endl;
	while (i < updates.first)
	{
		std::cout << "Socket " << updates.second[i].ident << " is available to read" << std::endl;
		std::cout << (updates.second[i].flags & EV_EOF) << std::endl;
		if (updates.second[i].filter == EVFILT_READ)
		{
			it = _sockets.find(updates.second[i].ident);
			//accept new connection
			if (it != _sockets.end())
			{
				new_fd = accept(it->first, &s_addr, &s_len);
				if (new_fd < 0)
				{
					std::cout << "error accepting connection" << std::strerror(errno) << std::endl;
					++i;
					continue;
				}
				_connections.insert(std::make_pair(new_fd, it));
				_kq.addFd(new_fd, true); //@todo catch error
			}
			if (it == _sockets.end())
			{
				ity = _connections.find(updates.second[i].ident);
				if (ity == _connections.end())
				{
					std::cout << "Error in connection logic" << std::endl; //@todo delete if statement here
					++i;
					continue;
				}
				std::string req(Utils::recv(updates.second[i].data, updates.second[i].ident));
				if (!req.empty())
				{
					HttpRequest request(req, Utils::ClientIpFromFd(updates.second[i].ident));
					_pending_response.insert(
							std::make_pair(updates.second->ident, ity->second->second.generate(request)));
				}
			}
			//client closing connection
			if (updates.second[i].flags & EV_EOF)
			{
				close(updates.second[i].ident);
				_kq.deleteFd(updates.second->ident, true); //@todo ?kqueue may clear itself on close(fd)?
				_connections.erase(updates.second[i].ident);
			}
		}
		i++;
	}
}

//HttpResponse Server::generateResponse(HttpRequest &request)
//{
//	HttpResponse response;
//	VirtualServer serv;
//
//	std::map<std::string, std::string>::iterator rq_it;
//	rq_it = request._header_fields.find("Host");
//	if (rq_it == request._header_fields.end())
//	{
//		//@todo return  400 ! implement error constructors for HttpResonse
//	}
//	for (std::map<int, VirtualServer>::iterator it = servers.begin(); it != servers.end(); ++it)
//	{
//		if (it->second.getServerName() == rq_it->second &&)
//		{
//
//		}
//	}
//
//}

void Server::process_response(std::pair<int, struct kevent *> &updates)
{
	int i = 0;
	std::map<int, HttpResponse>::iterator it;

	while (i < updates.first)
	{
		if (updates.second[i].filter == EVFILT_WRITE)
		{
			it = _pending_response.find(updates.second[i].ident);
			if (it != _pending_response.end())
			{
				it->second.sendResponse(it->first);
				_pending_response.erase(it);
				close(updates.second->ident);
				_kq.deleteFd(updates.second->ident, true);
				_connections.erase(updates.second[i].ident);
			}
		}
		++i;
	}
}

void Server::run(void)
{
	std::pair<int, struct kevent *> updates;

//	_kq.init();
	for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it)
	{
		std::cout << "Added " << it->first << " to Kqueue" << std::endl;
		_kq.addFd(it->first);
	}
	while (1) //@todo implement  gracefull exit and catching errors
	{

//		if (exit_flag)
//		{
//			return (EXIT_SUCCESS);
//		}
		updates = _kq.getUpdates();
		process_requests(updates);
		process_response(updates);
	}
}

bool Server::validate(const VirtualServer &server)
{
	if (server.validatePort() && server.validateHost() &&
		server.validateErrorPages() && server.validateLocations())
		return (true);
	std::cout << server.validateErrorPages() << std::endl;
	std::cout << server.validatePort() << std::endl;
	std::cout << server.validateHost() << std::endl;
	std::cout << server.validateLocations() << std::endl;

	throw ServerException("Config validation failed");
}

void Server::apply(VirtualServer &serv)
{
	for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it)
	{
		if (serv.getHost() == it->second.getIp() && serv.getPort() == it->second.getPort())
		{
			it->second.appendVirtualServer(serv);
			return;
		}
	}
	Socket sock(serv.getHost(), serv.getPort(), serv);
	_sockets.insert(std::make_pair(sock.getSocketFd(), sock));
}


Server &Server::operator=(const Server &rhs)
{
	if (this == &rhs)
		return (*this);
	_sockets = rhs._sockets;
	_connections = rhs._connections;
	_tok_list = rhs._tok_list;
	_pending_response = rhs._pending_response;
	_kq = rhs._kq;
	return (*this);
}

Server::Server(const Server &rhs) :
		_sockets(rhs._sockets),
		_connections(rhs._connections),
		_tok_list(rhs._tok_list),
		_pending_response(rhs._pending_response),
		_kq(rhs._kq)
{}


