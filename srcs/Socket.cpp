//
// Created by Elayne Debi on 9/30/21.
//

#include "Socket.hpp"

int Socket::getSocketFd() const {
    return _socket_fd;
}

void Socket::setSocketFd(int socketFd) {
    _socket_fd = socketFd;
}

in_addr_t Socket::getIp() const {
    return _ip;
}

void Socket::setIp(in_addr_t ip) {
    _ip = ip;
}

uint16_t Socket::getPort() const {
    return _port;
}

void Socket::setPort(uint16_t port) {
    _port = port;
}

const std::map<std::string, VirtualServer> &Socket::getVirtualServers() const {
    return _virtual_servers;
}

void Socket::appendVirtualServer(const VirtualServer &virtual_server) {
    bool empty = false;
    if (_virtual_servers.empty())
        empty = true;
    std::pair<std::map<std::string, VirtualServer>::iterator, bool> res;
    res = _virtual_servers.insert(std::make_pair(virtual_server.getServerName(), virtual_server));
    if (!res.second)
        throw SocketException("Duplicate virtual server");
    if (empty)
        _default_config = &(res.first->second);

}


Socket::SocketException::SocketException(const char *msg) : m_msg(msg) {}

Socket::SocketException::~SocketException() throw() {};

const char *Socket::SocketException::what() const throw() {
    std::cerr << "SocketError: ";
    return (m_msg);
}

Socket::Socket(in_addr_t ip,
               uint16_t port,
               int fd,
               VirtualServer &serv) :
        _socket_fd(fd),
        _ip(ip),
        _port(port), _default_config(nullptr) {
    appendVirtualServer(serv);
};

Socket::Socket(in_addr_t ip, uint16_t port, const VirtualServer &serv) :

        _ip(ip), _port(port) {
    appendVirtualServer(serv);
    sockaddr_in addr = {};
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
        throw SocketException(std::strerror(errno));
    fcntl(fd, F_SETFL, O_NONBLOCK);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port); //port number;
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
    in_addr b;
    b.s_addr = _ip;
    std::cout << "Created new socket on " << fd << " for ip " << inet_ntoa(b) << " port " << port << std::endl;
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        std::cout << "Bind again is not possible" << std::endl;
    }
}

void Socket::clear() {
    close(_socket_fd);
}

//@todo make sure virtual servers in each socket have unique server_name (only one empty server name per ip:port pair)
//@todo validate that each virtual server has at least one location and server root
HttpResponse Socket::generate(const HttpRequest &request) {
    std::map<std::string, std::string>::const_iterator it = request.getHeaderFields().find("Host");
    if (it == request.getHeaderFields().end()) {
        short i = 400;
        return (HttpResponse(i, *_default_config));
        //@todo generate 400
    }
    if (it->second.empty())
        _default_config->generate(request);
    //truncate port value in host value field
    std::string host_value = it->second.substr(0, it->second.find(':'));

    std::map<std::string, VirtualServer>::iterator ity;
    ity = _virtual_servers.find(it->second);
    if (ity == _virtual_servers.end())
        return (_default_config->generate(request));
    return (ity->second.generate(request));
}

Socket::~Socket() {
}

Socket &Socket::operator=(const Socket &rhs) {
    if (this == &rhs)
        return (*this);
    _socket_fd = rhs.getSocketFd();
    _ip = rhs.getIp();
    _port = rhs.getPort();
    _virtual_servers = rhs.getVirtualServers();
    _default_config = nullptr;
    for (std::map<std::string, VirtualServer>::iterator it = _virtual_servers.begin(); it != _virtual_servers.end(); ++it)
    {
        if (it->second == *rhs.getDefaultConfig())
        {
            _default_config = &it->second;
            break;
        }
    }
    return (*this);
}

Socket::Socket(const Socket &rhs) : _socket_fd(rhs._socket_fd),
                                    _ip(rhs._ip),
                                    _port(rhs._port),
                                    _virtual_servers(rhs._virtual_servers) {
    _default_config = nullptr;
    for (std::map<std::string, VirtualServer>::iterator it = _virtual_servers.begin(); it != _virtual_servers.end(); ++it)
    {
        if (it->second == *rhs.getDefaultConfig())
        {
            _default_config = &it->second;
            break;
        }
    }
}

VirtualServer *Socket::getDefaultConfig(void) const {
    return _default_config;
}