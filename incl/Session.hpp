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
    int _fd;
    Socket *_server_socket;
    HttpResponse *_response;
    HttpRequest *_request;
    struct sockaddr _s_addr;
    bool _keep_alive;
    bool _response_sent;
    time_t _connection_timeout;    //how long to live before initial http request (20 seconds by default);
    time_t _last_response_sent;  //keep connection open after last response
    enum {
        HTTP_DEFAULT_TIMEOUT = 60000000,
        HTTP_DEFAULT_CONNECTION_TIMEOUT = 30000000
    };

public:
    int getFd() const;

    void setFd(int fd);

    Socket *getServerSocket() const;

    void setServerSocket(Socket *serverSocket);

    const HttpResponse *getResponse() const;

    void setResponse(const HttpResponse *response);

    const HttpRequest *getRequest() const;

    void setRequest(HttpRequest *request);

    bool isKeepAlive() const;

    void setKeepAlive(bool keepAlive);

    bool isResponseSent() const;

    void setResponseSent(bool responseSent);

    time_t getConnectionTimeout() const;

    void setConnectionTimeout();

    time_t getLastResposnseSent() const;

    void setLastUpdate();

    Session();

    Session(const Session &rhs);

    Session &operator=(const Session &rhs);

    Session(Socket *sock);

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
