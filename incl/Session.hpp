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
#include "ISubscriber.hpp"
#include "IManager.hpp"

class HttpResponse;

class HttpRequest;

class Socket;

class ISubscriber;

class Session : public ISubscriber {
private:
    int              _fd;
public:
    int getFd() const;

private:
    Socket           *_server_socket;
    HttpResponse     *_response;
    HttpRequest      *_request;
    struct sockaddr  _s_addr;
    bool             _keep_alive;
    short            _status;
    time_t           _connection_timeout;
public:
    time_t getConnectionTimeout() const;

private:
    //how long to live before initial http request; //@todo add timeout header
    static const int HTTP_DEFAULT_TIMEOUT = 2;
    std::string      _buffer;
    IManager         *_mng;
public:

    enum Status {
        UNUSED         = 0,
        AWAIT_NEW_REQ  = 1, // if keep alive true close
        CGI_PROCESSING = 2,
        SENDING        = 3,// response ready -> send
        TIMEOUT        = 4,//should be closed with timeout;
        CLOSING        = 5
    };

    Session();

    Session(const Session &rhs);

    Session &operator=(const Session &rhs);

    Session(int i, Socket *pSocket, sockaddr sockaddr1, IManager *mng);

    virtual     ~Session();

    virtual void processEvent(int fd, size_t bytes_available, int16_t filter, uint32_t flags, bool eof, Server *serv);

    void processCurrentStatus(short status);

    void processPreviousStatus(short prev_status);

    bool shouldClose();

    short getStatus() const;

    void setStatus(short status);


    std::string getIpFromSock();


    void setResponse(HttpResponse *response);

    HttpRequest *getRequest() const;


    bool isKeepAlive() const;

    void setKeepAlive(bool keepAlive);

    bool writeCgi(size_t bytes, bool eof);

    bool readCgi(size_t bytes, bool eof);


    void parseRequest(size_t bytes);

    void prepareResponse();

    std::string getClientIp() const;

    std::string getClientPort() const;

    void end(void);

    void processResponse(size_t bytes, bool eof);

    class SessionException : public std::exception {
        const std::string m_msg;
    public:
        SessionException(const std::string &msg);

        ~SessionException() throw();

        const char *what() const throw();
    };

    void clearBuffer(void);

    std::string &getBuffer();

};

#endif //WEBSERV_SESSION_HPP
