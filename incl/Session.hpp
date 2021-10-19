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

class Socket;

class Session
{
private:
	int             _fd;
	Socket          *_server_socket;
	HttpResponse    *_response;
	HttpRequest     _request;
	struct sockaddr _s_addr;
	bool            _keep_alive;
	bool            _response_sent;
	time_t          _connection_timeout;    //time to live (if no updates from client received in this timeframe close connection
	time_t          _receive_timeout;  //if keep_alive false and not request has been made receive_timeout expired close connection; send request timeout
	time_t          _last_update; //server should send response in this timeframe
	enum
	{
		HTTP_DEFAULT_TIMEOUT            = 60000000,
		HTTP_DEFAULT_CONNECTION_TIMEOUT = 30000000
	};

public:
	int getFd() const;

	void setFd(int fd);

	Socket *getServerSocket() const;

	void setServerSocket(Socket *serverSocket);

	const HttpResponse *getResponse() const;

	void setResponse(const HttpResponse *response);

	const HttpRequest &getRequest() const;

	void setRequest(const HttpRequest &request);

	bool isKeepAlive() const;

	void setKeepAlive(bool keepAlive);

	bool isResponseSent() const;

	void setResponseSent(bool responseSent);

	time_t getConnectionTimeout() const;

	void setConnectionTimeout(time_t connectionTimeout);

	time_t getReceiveTimeout() const;

	void setReceiveTimeout(time_t receiveTimeout);

	time_t getLastUpdate() const;

	void setLastUpdate(time_t lastUpdate);

	Session();

	Session(const Session &rhs);

	Session &operator=(const Session &rhs);

	Session(Socket *sock);

	Session(int i, Socket *pSocket, sockaddr sockaddr1);

	void parseRequest(long bytes);

	void prepareResponse();

	~Session();

	void end(void);

	class SessionException : public std::exception
	{
		const std::string m_msg;
	public:
		SessionException(const std::string &msg);

		~SessionException() throw();

		const char *what() const throw();
	};

	void send();

	bool ifEnd(void);
};

#endif //WEBSERV_SESSION_HPP
