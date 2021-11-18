//
// Created by George Tevosov on 17.11.2021.
//

#ifndef WEBSERV_ISUBSCRIBER_HPP
#define WEBSERV_ISUBSCRIBER_HPP
#include "IManager.hpp"
#include "Server.hpp"
class Server;
class ISubscriber {
public:
    virtual ~ISubscriber() = 0;
    virtual void processEvent(int fd, size_t bytes_available, int16_t filter, bool eof, Server *serv) = 0;
    virtual bool shouldClose() = 0;

};
#endif //WEBSERV_ISUBSCRIBER_HPP
