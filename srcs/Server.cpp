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
    for (std::map<int, Socket*>::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
        it->second->clear();
    }
}

void signal_handler(int signal) {
    std::cout << "YO" << std::endl;
//	sleep(10);
    exit(signal);
}

Server::Server(const std::string &config_file) : _kq(MAX_KQUEUE_EV) {
    signal(SIGPIPE, SIG_IGN);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGQUIT, signal_handler);
    Utils::tokenizeFileStream(config_file, _tok_list);
    std::list<std::string>::iterator     end = _tok_list.end();
    std::list<std::string>::iterator     it  = _tok_list.begin();
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
            ++it;
            //@todo THROW ERROR if nothing else worked
        }
        MimeType("/Users/edebi/Desktop/webserv/mime.conf"); //@todo put mime path to config
        validate(serv);
        apply(serv);
    }
    for (std::map<int, Socket*>::iterator ity = _sockets.begin(); ity != _sockets.end(); ++ity) {
        std::cout << "Socket created on fd " << ity->first << " internal " << ity->second->getSocketFd() << std::endl;
    }
    Server::loop();
}


//void Server::acceptConnection(std::map<int, Socket>::iterator it) {
//    std::pair<int, Session *> session;
//
//    session = it->second.acceptConnection();
//    _sessions.insert(session);
//    _kq.addFd(session.first, true);
//}

//void Server::prepareResponse(std::map<int, Session *>::iterator sess_iter, long bytes) {
//    if (sess_iter == _sessions.end()) {
//        std::cout << "Should never happend" << std::endl;
//        return;
//    }
//    sess_iter->second->parseRequest(bytes);
//
//    if (sess_iter->second->getRequest()->isReady()) {
//        sess_iter->second->prepareResponse();
//        if (sess_iter->second->getStatus() == Session::PIPE_TO_CGI) {
//            watchPipe(sess_iter->second, true);
////            _cgi_pipes.insert(
////                    std::map<int, Session *>::value_type(
////                            sess_iter->second->getResponse()->getCgi()->getRequestPipe(), sess_iter->second));
////            if (sess_iter->second->writeCgi())
////                _pending_sessions.insert(sess_iter->first);
//        } else if (sess_iter->second->getStatus() == Session::READ_FROM_CGI)
//            watchPipe(sess_iter->second, false);
//        else
//            _pending_sessions.insert(sess_iter->first);
//    }
//}

void Server::closeConnection(int cur_fd) {
    std::map<int, Session *>::iterator sess_iter;

    sess_iter = _sessions.find(cur_fd);
    if (sess_iter == _sessions.end())
        throw ServerException("invalid fd in close connection");
    sess_iter->second->end();
    _sessions.erase(cur_fd);
//    _pending_sessions.erase(cur_fd);
    _kq.deleteFd(cur_fd, true);
}
//
///* false - read, true - write*/
//void Server::watchPipe(Session *sess, bool rw) {
//    if (rw) {
//        _kq.addWriteOnly(sess->getResponse()->getCgi()->getRequestPipe());
//        _cgi_pipes.insert(std::map<int, Session *>::value_type(sess->getResponse()->getCgi()->getRequestPipe(), sess));
//    } else {
//        _kq.addFd(sess->getResponse()->getCgi()->getResponsePipe());
//        _cgi_pipes.insert(std::map<int, Session *>::value_type(sess->getResponse()->getCgi()->getResponsePipe(), sess));
//    }
//}
//
//void Server::processReadEvent(struct kevent event) {
//
//    uint16_t                           cur_flags = event.flags;
//    int                                cur_fd    = event.ident;
//    std::map<int, Socket>::iterator    it        = _sockets.find(cur_fd);
//    std::map<int, Session *>::iterator sess_it;
//
//    //accept new connection
//    if (it != _sockets.end())
//        acceptConnection(it);
//    else if ((sess_it = _cgi_pipes.find(cur_fd)) != _cgi_pipes.end() &&
//             sess_it->second->getStatus() == Session::READ_FROM_CGI) {
//        if (sess_it->second->readCgi(event.data, (cur_flags & EV_EOF))) //@todo ready to send
//        {
//            _cgi_pipes.erase(sess_it);
//            _pending_sessions.insert(sess_it->second->getFd());
//            return;
//        }
////@todo read response from pipe. If read complete remove;
//    } else if (event.data != 0) {
//        sess_it = _sessions.find(cur_fd);
//        if (sess_it != _sessions.end()) {
//            prepareResponse(_sessions.find(cur_fd), event.data);
//            if (sess_it->second->getStatus() == Session::PIPE_TO_CGI)
//                watchPipe(sess_it->second, true);
//            else if (sess_it->second->getStatus() == Session::READ_FROM_CGI) {
//                watchPipe(sess_it->second, false);
//            }
//        }
//    }
//    if (cur_flags & EV_EOF)
//        closeConnection(cur_fd);
//}

//void Server::processWriteEvent(struct kevent event) {
//    std::map<int, Session *>::iterator it;
//    int                                cur_fd;
//
//    cur_fd = event.ident;
//    if (_pending_sessions.find(cur_fd) != _pending_sessions.end()) {
//        it = _sessions.find(cur_fd);
////        if (it->second->getStatus() == Session::READY_TO_SEND) {
//        it->second->send();
//        _pending_sessions.erase(cur_fd);
////        }
//        if (it->second->shouldClose())
//            closeConnection(cur_fd);
//    } else if ((it = _cgi_pipes.find(cur_fd)) != _cgi_pipes.end() &&
//               it->second->getStatus() == Session::PIPE_TO_CGI) {
//        if (it->second->writeCgi(event.data, event.flags & EV_EOF)) {
//            _cgi_pipes.erase(it->second->getResponse()->getCgi()->getRequestPipe());
//            watchPipe(it->second, false);
//        }
//    } else {
//        it = _sessions.find(cur_fd);
//        if (it != _sessions.end() && it->second->shouldClose()) {
//            it->second->end();
//            _sessions.erase(cur_fd);
//            _pending_sessions.erase(cur_fd);
//        } else if (it != _sessions.end()
//                   && it->second->getRequest() != nullptr &&
//                   it->second->getRequest()->getParsingError() == HttpResponse::HTTP_CONTINUE) {
//            it->second->getRequest()->sendContinue(cur_fd);
//            it->second->getRequest()->setParsingError(0);
//            it->second->getRequest()->setReady(false);
//        }
//    }
//}

//void Server::loopEvenets(std::pair<int, struct kevent *> &updates) {
//    int i = 0;
//
////	std::cout << "Kqueue update size " << updates.first << std::endl;
//    while (i < updates.first) {
//        if (updates.second[i].filter == EVFILT_READ) {
//            processReadEvent(updates.second[i]);
//        }
//        if (updates.second[i].filter == EVFILT_WRITE)
//            processWriteEvent(updates.second[i]);
//        i++;
//    }
//}

/*
void Server::processRequests(std::pair<int, struct kevent *> &updates) {
    int                                i = 0;
    std::map<int, Socket>::iterator    it;
    std::map<int, Session *>::iterator sess_iter;
    int16_t                            cur_filter;
    uint16_t                           cur_flags;
    int                                cur_fd;

//	std::cout << "Kqueue update size " << updates.first << std::endl;
    while (i < updates.first) {
        cur_filter = updates.second[i].filter;
        cur_fd     = updates.second[i].ident;
        cur_flags  = updates.second[i].flags;
        if (cur_filter == EVFILT_READ) {
            it = _sockets.find(cur_fd);
            //accept new connection
            if (it != _sockets.end())
                acceptConnection(it);
            else if (updates.second[i].data != 0)
                prepareResponse(_sessions.find(cur_fd), updates.second[i].data);
            if (cur_flags & EV_EOF)
                closeConnection(cur_fd);
        }
        i++;
    }
}


void Server::processResponse(std::pair<int, struct kevent *> &updates) {
    int                                i = 0;
    std::map<int, Session *>::iterator it;
    int                                cur_fd;

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
                    _pending_sessions.erase(cur_fd);
                } else if (c_it != _sessions.end()
                           && c_it->second->getRequest() != nullptr &&
                           c_it->second->getRequest()->getParsingError() == HttpResponse::HTTP_CONTINUE) {
                    c_it->second->getRequest()->sendContinue(cur_fd);
                    c_it->second->getRequest()->setParsingError(0);
                    c_it->second->getRequest()->setReady(false);
                }
            }
        }
        ++i;
    }
}
//*/
//void Server::run(void) {
//    std::pair<int, struct kevent *> updates;
//
//    for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
//        std::cout << "Added " << it->first << " to Kqueue" << std::endl;
//        _kq.addFd(it->first);
//    }
//    while (1) //@todo implement  gracefull exit and catching errors
//    {
////		if (exit_flag)
////		{
////			return (EXIT_SUCCESS);
////		}
//        updates = _kq.getUpdates();
//        loopEvenets(updates);
////        processRequests(updates);
////        processResponse(updates);
//    }
//}

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
    for (std::map<int, Socket*>::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
        if (serv.getHost() == it->second->getIp() && serv.getPort() == it->second->getPort()) {
            it->second->appendVirtualServer(serv);
            return;
        }
    }
//    Socket                               sock(serv.getHost(), serv.getPort(), serv, this);
    Socket *sock = new  Socket(serv.getHost(), serv.getPort(), serv, this);
//    std::make_pair(sock->getSocketFd(), sock)
    _sockets.insert(std::map<int, Socket*>::value_type(sock->getSocketFd(), sock));
}


Server &Server::operator=(const Server &rhs) {
    if (this == &rhs)
        return (*this);
    _sockets  = rhs._sockets;
    _sessions = rhs._sessions;
//    _pending_sessions = rhs._pending_sessions;
    _tok_list = rhs._tok_list;
    _kq       = rhs._kq;
    return (*this);
}

Server::Server(
        const Server &rhs) :
        _sockets(rhs._sockets),
        _sessions(rhs._sessions),
//        _pending_sessions(rhs._pending_sessions),
        _tok_list(rhs._tok_list),
        _kq(rhs._kq) {}

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

_Noreturn void Server::loop() {
    std::pair<int, struct kevent *>        updates;
    std::map<int, ISubscriber *>::iterator it;
    int                                    i;
    size_t                                 bytes_available;
    int                                    cur_fd;
    int16_t                                filter;
    uint16_t                               flags;

    while (true) {
        updates = _kq.getUpdates();
        i       = 0;
        while (i < updates.first) {
            it              = _subs.find(updates.second[i].ident);
            filter          = updates.second[i].filter;
            cur_fd          = updates.second[i].ident;
            flags           = updates.second[i].flags;
            bytes_available = updates.second[i].data;
            if (it == _subs.end())
                std::cout << "SHOULD NEVER HAPPEND TYPE OF ERROR" << std::endl;
            else
                it->second->processEvent(cur_fd, bytes_available, filter, (flags & EV_EOF), this);
            ++i;
        }
        removeExpiredSessions();

    }

}

void Server::addSession(std::pair<int, Session *> pair) {
    _sessions.insert(pair);
}

void Server::monitorSession(int fd, Session *sess) {
    _sessions.insert(std::map<int, Session *>::value_type(fd, sess));
}


