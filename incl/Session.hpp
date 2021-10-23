//
// Created by George Tevosov on 17.10.2021.
//

#ifndef WEBSERV_SESSION_HPP
#define WEBSERV_SESSION_HPP

#include <string>
#include <iostream>
#include <ctime>
#include <sys/socket.h>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <unistd.h>
#include "Socket.hpp"

class HttpResponse;

class HttpRequest;

class Socket;

class Session {
private:
    int              _fd;
    Socket           *_server_socket;
    HttpResponse     *_response;
    HttpRequest      *_request;
    struct sockaddr  _s_addr;
    bool             _keep_alive;
    short            _status;
    time_t           _connection_timeout;    //how long to live before initial http request (20 seconds by default);
    static const int HTTP_DEFAULT_TIMEOUT = 5;
    enum Status {
        UNUSED        = 1, // if keep_alive false -> do not close
        AWAIT_NEW_REQ = 2, // if keep alive true close
        READY_TO_SEND = 3, // response ready -> send
        TIMEOUT       = 4, //should be closed with timeout;
    };

public:

    short getStatus() const;

    void setStatus(short status);

    int getFd() const;

    void setFd(int fd);

    Socket *getServerSocket() const;

    std::string getIpFromSock();

    void setServerSocket(Socket *serverSocket);

    const HttpResponse *getResponse() const;

    void setResponse(const HttpResponse *response);

    HttpRequest *getRequest() const;

    void setRequest(HttpRequest *request);

    bool isKeepAlive() const;

    void setKeepAlive(bool keepAlive);

    time_t getConnectionTimeout() const;

    void setConnectionTimeout();

    Session();

    Session(const Session &rhs);

    Session &operator=(const Session &rhs);

    Session(int i, Socket *pSocket, sockaddr sockaddr1);

    void parseRequest(long bytes);

    void prepareResponse();

    ~Session();

    void end(void);

    class SessionException : public std::exception {
        const std::string m_msg;
    public:
        SessionException(const std::string &msg);

        ~SessionException() throw();

        const char *what() const throw();
    };

    void send();

    bool shouldClose(void);
};

#endif //WEBSERV_SESSION_HPP
