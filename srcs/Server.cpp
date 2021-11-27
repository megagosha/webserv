//
// Created by Elayne Debi on 9/9/21.
//
#include "Server.hpp"

/*
 * @todo
 * The select or euqivalent should be in the main loop and should check fd for read and write at the same time
 * If read or write return error client should be disconnected
 * You should check both -1 and 0
 * For TCP sockets, the return value 0 means the peer has closed its half side of the connection.
 *
 */
void signal_handler(int signal) {
    std::cout << "stopping on signal " << signal << std::endl;
    exit(signal);
}

Server::Server(const std::string &config_file) : _kq(MAX_KQUEUE_EV) {
    try {
        registerSignal();
        parseConfig(config_file);
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
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


Server::~Server() {
    for (std::map<int, Socket *>::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
        it->second->clear();
    }
}

void Server::parseConfig(const std::string &config) {
    Utils::tokenizeFileStream(config, _tok_list);
    std::list<std::string>::iterator end = _tok_list.end();
    std::list<std::string>::iterator it  = _tok_list.begin();
    std::string                      mime_conf_path;
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
            if (it != end && *it == "mime_conf_path") {
                Utils::skipTokens(it, end, 1);
                mime_conf_path = *it;
            }
            ++it;
            //@todo THROW ERROR if nothing else worked
        }
        if (mime_conf_path.empty())
            throw Server::ServerException("No mime.conf path");
        MimeType(mime_conf_path.c_str());
        validate(serv);
        apply(serv);
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
    uint32_t                               fflags;
    while (true) {
        updates = _kq.getUpdates();
        i       = 0;
        while (i < updates.first) {
            it              = _subs.find(updates.second[i].ident);
            filter          = updates.second[i].filter;
            cur_fd          = updates.second[i].ident;
            flags           = updates.second[i].flags;
            fflags          = updates.second[i].fflags;
            bytes_available = updates.second[i].data;
            if (flags & EV_ERROR) {   /* report any error */
                fprintf(stderr, "EV_ERROR: %s\n", strerror(bytes_available));
            }
            if (it != _subs.end())
                it->second->processEvent(cur_fd, bytes_available, filter, fflags, (flags & EV_EOF), this);
            ++i;
        }
        removeExpiredSessions();
    }
}

void Server::apply(VirtualServer &serv) {
    for (std::map<int, Socket *>::iterator it    = _sockets.begin(); it != _sockets.end(); ++it) {
        if (serv.getHost() == it->second->getIp() && serv.getPort() == it->second->getPort()) {
            it->second->appendVirtualServer(serv);
            return;
        }
    }
    Socket                                 *sock = new Socket(serv.getHost(), serv.getPort(), serv, this);
    _sockets.insert(std::map<int, Socket *>::value_type(sock->getSocketFd(), sock));
}

void Server::subscribe(int fd, short type, ISubscriber *obj) {
    try {
        _kq.addFd(fd, type);
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    _subs.insert(std::map<int, ISubscriber *>::value_type(fd, obj));
}

void Server::unsubscribe(int fd, int16_t type) {
    _kq.deleteFd(fd, type);
    _subs.erase(fd);
}

void Server::removeSession(int fd) {
    _subs.erase(fd);
    _sessions.erase(fd);
}

void Server::removeExpiredSessions() {
    if (_sessions.empty())
        return;
    std::vector<Session *> to_delete;
    to_delete.reserve(_sessions.size());
    for (std::map<int, Session *>::iterator sess_it = _sessions.begin(); sess_it != _sessions.end(); ++sess_it) {
        if (sess_it->second->getStatus() == Session::AWAIT_NEW_REQ &&
            sess_it->second->shouldClose()) {
            to_delete.push_back(sess_it->second);
        }
    }
    for (std::vector<Session *>::iterator   vec_it  = to_delete.begin(); vec_it != to_delete.end(); ++vec_it) {
        (*vec_it)->end();
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

void Server::addSession(std::pair<int, Session *> pair) {
    _sessions.insert(pair);
}

void Server::registerSignal() {
    signal(SIGPIPE, SIG_IGN);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGQUIT, signal_handler);
}

Server::ServerException::ServerException(const std::string &msg) : m_msg(msg) {}

Server::ServerException::~ServerException() throw() {}

const char *Server::ServerException::what() const throw() {
    std::cerr << "ServerError: " << m_msg << std::endl;
    return (std::exception::what());
}
