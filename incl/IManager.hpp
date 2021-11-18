//
// Created by George Tevosov on 17.11.2021.
//

#ifndef WEBSERV_IMANAGER_HPP
#define WEBSERV_IMANAGER_HPP
#include "ISubscriber.hpp"

class ISubscriber;
class IManager {
public:
    virtual void subscribe(int fd, int16_t type, ISubscriber *obj) = 0;
    virtual void unsubscribe(int fd, int16_t type) = 0;
    virtual void loop() = 0;
};
#endif //WEBSERV_IMANAGER_HPP
