//
// Created by Elayne Debi on 9/9/21.
//
#include "Server.hpp"


Server::ServerException::ServerException(const std::string &msg) : m_msg(msg) {}

Server::ServerException::~ServerException() throw() {};

const char *Server::ServerException::what() const throw() {
    std::cerr << "ServerError: ";
    return (std::exception::what());
}


//@todo create destructor
Server::~Server() {
    for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
        it->second.clear();
    }
}

Server::Server(const std::string &config_file) : _kq(MAX_KQUEUE_EV) {
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
            if (it != end && *it == "location") {
                serv.setLocation(it, end);
                continue;
            }
            ++it;
            //@todo THROW ERROR if nothing else worked
        }
        MimeType("/Users/edebi/Desktop/webserv/mime.conf"); //@todo put mime path to config
        validate(serv);
        apply(serv);
    }
    for (std::map<int, Socket>::iterator ity = _sockets.begin(); ity != _sockets.end(); ++ity) {
        std::cout << "Socket created on fd " << ity->first << " internal " << ity->second.getSocketFd() << std::endl;
    }
    run();
}

void Server::acceptConnection(std::map<int, Socket>::iterator it) {
    std::pair<int, Session *> session;

    session = it->second.acceptConnection();
    _sessions.insert(session);
    _kq.addFd(session.first, true);
}

void Server::prepareResponse(std::map<int, Session *>::iterator sess_iter, long bytes) {
    if (sess_iter == _sessions.end()) {
        std::cout << "Should never happend" << std::endl;
        return;
    }
    sess_iter->second->parseRequest(bytes);
    if (sess_iter->second->getRequest()->isReady()) {
        sess_iter->second->prepareResponse();
        _pending_sessions.insert(sess_iter->first);
    }
}

void Server::closeConnection(int cur_fd) {
    std::map<int, Session *>::iterator sess_iter;

    sess_iter = _sessions.find(cur_fd);
    if (sess_iter == _sessions.end())
        throw ServerException("WTF");
    sess_iter->second->end();
    _sessions.erase(cur_fd);
    _pending_sessions.erase(cur_fd);
    _kq.deleteFd(cur_fd, true);
}

void Server::processRequests(std::pair<int, struct kevent *> &updates) {
    int i = 0;
    std::map<int, Socket>::iterator it;
    std::map<int, Session *>::iterator sess_iter;
    int16_t cur_filter;
    uint16_t cur_flags;
    int cur_fd;

//	std::cout << "Kqueue update size " << updates.first << std::endl;
    while (i < updates.first) {
        cur_filter = updates.second[i].filter;
        cur_fd = updates.second[i].ident;
        cur_flags = updates.second[i].flags;
        if (cur_filter == EVFILT_READ) {
            it = _sockets.find(cur_fd);
            //accept new connection
            if (it != _sockets.end())
                acceptConnection(it);
            else
                prepareResponse(_sessions.find(cur_fd), updates.second[i].data);
            if (cur_flags & EV_EOF)
                closeConnection(cur_fd);
        }
        i++;
    }
}


void Server::processResponse(std::pair<int, struct kevent *> &updates) {
    int i = 0;
    std::map<int, Session *>::iterator it;
    int cur_fd;

    while (i < updates.first) {
        if (updates.second[i].filter == EVFILT_WRITE) {
            cur_fd = updates.second[i].ident;
            if (_pending_sessions.find(cur_fd) != _pending_sessions.end()) {
                it = _sessions.find(cur_fd);
                std::cout << "keep alive " << it->second->isKeepAlive() << std::endl;
                it->second->send();
                if (it->second->shouldClose())
                    closeConnection(cur_fd);
                _pending_sessions.erase(cur_fd);

            } else {
                std::map<int, Session *>::iterator c_it;
                c_it = _sessions.find(cur_fd);
                if (c_it != _sessions.end() && c_it->second->shouldClose()) {
                    c_it->second->end();
                    _sessions.erase(cur_fd);
                }
            }
        }
        ++i;
    }
}

void Server::run(void) {
    std::pair<int, struct kevent *> updates;

    for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
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
        processRequests(updates);
        processResponse(updates);
    }
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

void Server::apply(VirtualServer &serv) {
    for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
        if (serv.getHost() == it->second.getIp() && serv.getPort() == it->second.getPort()) {
            it->second.appendVirtualServer(serv);
            return;
        }
    }
    Socket sock(serv.getHost(), serv.getPort(), serv);
    _sockets.insert(std::make_pair(sock.getSocketFd(), sock));
}


Server &Server::operator=(const Server &rhs) {
    if (this == &rhs)
        return (*this);
    _sockets = rhs._sockets;
    _sessions = rhs._sessions;
    _pending_sessions = rhs._pending_sessions;
    _tok_list = rhs._tok_list;
    _kq = rhs._kq;
    return (*this);
}

Server::Server(const Server &rhs) :
        _sockets(rhs._sockets),
        _sessions(rhs._sessions),
        _pending_sessions(rhs._pending_sessions),
        _tok_list(rhs._tok_list),
        _kq(rhs._kq) {}


