//
// Created by Elayne Debi on 9/30/21.
//


#ifndef WEBSERV_SOCKET_HPP
#define WEBSERV_SOCKET_HPP

#include "VirtualServer.hpp"
#include <map>
#include <string>
#include <unistd.h>

class Socket {
private:
    int _socket_fd;
    in_addr_t _ip;
    uint16_t _port;
    VirtualServer &_default_config;
    std::map<std::string, VirtualServer> _virtual_servers; //@todo change container
public:
    int getSocketFd() const {
        return _socket_fd;
    }

    void setSocketFd(int socketFd) {
        _socket_fd = socketFd;
    }

    in_addr_t getIp() const {
        return _ip;
    }

    void setIp(in_addr_t ip) {
        _ip = ip;
    }

    uint16_t getPort() const {
        return _port;
    }

    void setPort(uint16_t port) {
        _port = port;
    }

    const std::map<std::string, VirtualServer> &getVirtualServers() const {
        return VirtualServers;
    }

    void appendVirtualServer(const VirtualServer &virtual_server) const {
        if (_virtual_servers.empty())
            _default_config = virtual_server;
        std::pair<_virtual_servers::iterator, bool> res;
        res = _virtual_servers.insert(std::make_pair(virtual_server.getServerName(), virtual_server));
        if (res->second == false)
            throw SocketException("Duplicate virtual server");
    }

    Socket();

public:
    class SocketException : public std::exception {
        const char *m_msg;
    public:
        explicit SocketException(const char *msg) : m_msg(msg) {}

        ~SocketException() throw() {};

        const char *what() const throw() {
            std::cerr << "SocketError: ";
            return (m_msg);
        }
    };

    Socket(uint32_t ip, uint16_t port, int fd) : _ip(ip), _port(port), _socket_fd(fd) {};

    Socket(uint32_t ip, uint16_t port) : _ip(ip), _port(port) {
        sockaddr_in addr = {};
        int fd;

        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd < 0)
            throw SocketException(std::strerror(errno));
        fcntl(fd, F_SETFL, O_NONBLOCK);
        addr.sin_family = AF_INET;
        addr.sin_port = _port; //port number;
        addr.sin_addr.s_addr = _ip;
        memset(addr.sin_zero, 0, 8);
        addr.sin_len = sizeof(addr);
        if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
            close(fd);
            throw SocketException(std::strerror(errno));
        }
        if (listen(fd, MAX_AWAIT_CONN) == -1) {
            close(fd);
            throw SocketException(std::strerror(errno));
        }
        _socket_fd = fd;
    }

    void clear() {
        close(_socket_fd);
    }

    HttpResponse generate(const HttpRequest &request)
    {
        std::map<std::string, std::string>::iterator it = request.header_fields.find("Host");
        if (it == request.header_fields.end())
        {
            return (HttpResponse(400));
            //@todo generate 400
        }
        if (it->second.empty())
            _default_config.generate(request);
        if ()
        std::map<std::string, VirtualServer>::iterator ity;
        ity = _virtual_servers.find(it->second);
        if (ity == _virtual_servers.end())
           return (_default_config.generate(request));
        return (ity->second->generate(request));
    }
    ~Socket() {
    }

};

#endif //WEBSERV_SOCKET_HPP
