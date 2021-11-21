//
// Created by Elayne Debi on 9/9/21.
//
#include "Server.hpp"

void signal_handler(int signal) {
	std::cout << "stopping on signal " << signal << std::endl;
	exit(signal);
}

Server::Server(const std::string &config_file) : _kq(MAX_KQUEUE_EV) {
    signal(SIGPIPE, SIG_IGN);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGQUIT, signal_handler);
    Utils::tokenizeFileStream(config_file, _tok_list);
    std::list<std::string>::iterator     end = _tok_list.end();
    std::list<std::string>::iterator     it  = _tok_list.begin();
	std::string mime_conf_path;
	for (; it != end; ++it) // loop for servers
	{
        if (*it != "server" || *(++it) != "{")
            throw Server::ServerException("Error while parsing config file");
        VirtualServer serv;
        ++it;
        std::list<std::string>::iterator check;
        while (it != end && *it != "}") //loop inside server block
        {
            if (it != end && *it == "listen") {
                serv.setHost(it, end);
                continue;
            }
            if (it != end && *it == "port") {
                serv.setPort(it, end);
                continue;
            }
            if (it != end && *it == "client_max_body_size") {
                serv.setBodySize(it, end);
                continue;
            }
            if (it != end && *it == "error_page") {
                serv.setErrorPage(it, end);
                continue;
            }
            if (it != end && *it == "server_name") {
                serv.setServerName(it, end);
                continue;
            }
            if (it != end && *it == "location") {
                serv.setLocation(it, end);
                continue;
            }
			if (it != end && *it == "mime_conf_path")
			{
				Utils::skipTokens(it, end, 1);
				mime_conf_path = *it;
			}
			++it;
            //@todo THROW ERROR if nothing else worked
        }
		if(mime_conf_path.empty())
			throw Server::ServerException("No mime.conf path");
		MimeType(mime_conf_path.c_str());
		validate(serv);
        apply(serv);
    }
    for (std::map<int, Socket*>::iterator ity = _sockets.begin(); ity != _sockets.end(); ++ity) {
        std::cout << "Socket created on fd " << ity->first << " internal " << ity->second->getSocketFd() << std::endl;
    }
    Server::loop();
}


Server &Server::operator=(const Server &rhs) {
    if (this == &rhs)
        return (*this);
    _sockets  = rhs._sockets;
    _sessions = rhs._sessions;
    _tok_list = rhs._tok_list;
    _kq       = rhs._kq;
    return (*this);
}

Server::Server(
        const Server &rhs) :
        _sockets(rhs._sockets),
        _sessions(rhs._sessions),
        _tok_list(rhs._tok_list),
        _kq(rhs._kq) {}


//@todo create destructor
Server::~Server() {
    for (std::map<int, Socket*>::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
        it->second->clear();
    }
}

void Server::loop() {
    std::pair<int, struct kevent *>        updates;
    std::map<int, ISubscriber *>::iterator it;
    int                                    i;
    size_t                                 bytes_available;
    int                                    cur_fd;
    int16_t                                filter;
    uint16_t                               flags;
    uint32_t                                fflags;
    while (true) {
        updates = _kq.getUpdates();
        i       = 0;
        while (i < updates.first) {
            it              = _subs.find(updates.second[i].ident);
            filter          = updates.second[i].filter;
            cur_fd          = updates.second[i].ident;
            flags           = updates.second[i].flags;
            fflags =           updates.second[i].fflags;
            bytes_available = updates.second[i].data;
            if (it == _subs.end())
                std::cout << "SHOULD NEVER HAPPEND TYPE OF ERROR" << std::endl;
            else
                it->second->processEvent(cur_fd, bytes_available, filter, fflags, (flags & EV_EOF), this);
            ++i;
        }
        removeExpiredSessions();
    }
}

void Server::apply(VirtualServer &serv) {
    for (std::map<int, Socket*>::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
        if (serv.getHost() == it->second->getIp() && serv.getPort() == it->second->getPort()) {
            it->second->appendVirtualServer(serv);
            return;
        }
    }
    Socket *sock = new  Socket(serv.getHost(), serv.getPort(), serv, this);
    _sockets.insert(std::map<int, Socket*>::value_type(sock->getSocketFd(), sock));
}

void Server::subscribe(int fd, short type, ISubscriber *obj) {
    _kq.addFd(fd, type);
    _subs.insert(std::map<int, ISubscriber *>::value_type(fd, obj));
}

void Server::unsubscribe(int fd, int16_t type) {
    _kq.deleteFd(fd, type);
//    if (type == EVFILT_WRITE && _sessions.find(fd) == _sessions.end())
    _subs.erase(fd);
//    _pending_sessions.erase(fd);
}

void Server::removeSession(int fd)
{
    _sessions.erase(fd);
    _subs.erase(fd);
}

void Server::removeExpiredSessions() {
    std::vector<int> to_delete;
    to_delete.reserve(_sessions.size());
    for (std::map<int, Session *>::iterator sess_it = _sessions.begin(); sess_it != _sessions.end(); ++sess_it) {
        if (sess_it->second->shouldClose()) {
            sess_it->second->end();
            to_delete.push_back(sess_it->first);
        }
    }
    for (std::vector<int>::iterator         vec_it  = to_delete.begin(); vec_it != to_delete.end(); ++vec_it)
        _sessions.erase(*vec_it);
}


bool Server::validate(const VirtualServer &server) {
    if (server.validatePort() && server.validateHost() &&
        server.validateErrorPages() && server.validateLocations())
        return (true);
    std::cout << "Error pages found " << server.validateErrorPages() << std::endl;
    std::cout << "Port is valid " << server.validatePort() << std::endl;
    std::cout << "Host is valid " << server.validateHost() << std::endl;
    std::cout << "Locations are valid " << server.validateLocations() << std::endl;

    throw ServerException("Config validation failed");
}

void Server::addSession(std::pair<int, Session *> pair) {
    _sessions.insert(pair);
}


Server::ServerException::ServerException(const std::string &msg) : m_msg(msg) {}

Server::ServerException::~ServerException() throw() {}

const char *Server::ServerException::what() const throw() {
    std::cerr << "ServerError: ";
    return (std::exception::what());
}
