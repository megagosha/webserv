//
// Created by Elayne Debi on 9/9/21.
//

#ifndef SERVER_HPP
#define SERVER_HPP

#include "VirtualServer.hpp"
#include "ISubscriber.hpp"
#include "IManager.hpp"
#include "Socket.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "KqueueEvents.hpp"
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include "Utils.hpp"
#include <list>
#include "Session.hpp"
#include "MimeType.hpp"

#define MAX_AWAIT_CONN 10000
#define MAX_KQUEUE_EV 300

class Socket;

class VirtualServer;

class Server : public IManager {

private:
    std::map<int, Socket *>      _sockets; // <socket fd, socket obj>
    std::map<int, Session *>     _sessions;
    std::map<int, ISubscriber *> _subs;
    std::list<std::string>       _tok_list;
    KqueueEvents                 _kq;

    Server();

public:

    Server &operator=(const Server &rhs);

    Server(const Server &rhs);

    //@todo create destructor
    ~Server();

    Server(const std::string &config_file);

    void addSession(std::pair<int, Session *> pair);

    virtual void subscribe(int fd, short type, ISubscriber *obj);

    virtual void unsubscribe(int fd, int16_t type);

    virtual void loop();

    bool validate(const VirtualServer &server);

    void apply(VirtualServer &serv);

    void removeExpiredSessions();

    void removeSession(int fd);

    class ServerException : public std::exception {
        const std::string m_msg;
    public:
        ServerException(const std::string &msg);

        ~ServerException() throw();

        const char *what() const throw();
    };
};

#endif //SERVER_HPP
