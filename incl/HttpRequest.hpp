//
// Created by Elayne Debi on 9/9/21.
//

//3 types of bodies
// fixedlength
// _chunked transfer
// no length provided

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <set>
#include <sstream>
#include <map>
#include <iostream>
#include "Utils.hpp"
#include "HttpResponse.hpp"
#include "Socket.hpp"
#include "Location.hpp"
#include "Session.hpp"
enum Limits
{
	MAX_METHOD            = 8,
	MAX_URI               = 2000,
	MAX_V                 = 100,
	MAX_FIELDS            = 100,
	MAX_NAME              = 100,
	MAX_VALUE             = 1000,
	MAX_MESSAGE			  = 10000,
	MAX_DEFAULT_BODY_SIZE = 1000000
};

std::string &leftTrim(std::string &str, std::string chars);

class Socket;

class Location;
class Session;
class HttpRequest
{
private:
	std::string                        _method;
	std::string                        _request_uri;
	std::string                        _query_string;
	std::string                        _uri_no_query;
	std::string                        _normalized_path;
	std::string                        _http_v; //true if http/1.1
	bool                               _chunked;
	std::map<std::string, std::string> _header_fields;
	std::string                        _body;
	std::string                        _client_ip;
	unsigned long                      _content_length;
	bool                               _ready;
	unsigned long                      _max_body_size;
	uint16_t                           _parsing_error;
	std::string 						_chunk_length;
	unsigned long 						_c_bytes_left;
	short								_skip_n;
//	std::string							_current_chunk;

public:
	void setNormalizedPath(const std::string &normalizedPath);

	const std::string &getUriNoQuery() const;

	void processUri(void);

	unsigned long getContentLength() const;

	void sendContinue(int fd);

	bool isReady() const;

	void setReady(bool ready);

	uint16_t getParsingError() const;

	void setParsingError(uint16_t parsingError);

	const std::string &getClientIp() const;

	HttpRequest();

	HttpRequest(const HttpRequest &rhs);

	HttpRequest &operator=(const HttpRequest &rhs);

	//reserve field memory
	HttpRequest(Session *sess, const std::string &client_ip, unsigned long bytes);

	bool parseChunked(Session *sess, unsigned long &pos, unsigned long bytes);

	bool appendBody(Session *sess, size_t &pos);

	const std::string &getMethod() const;

	const std::string &getRequestUri() const;


	const std::string &getHttpV() const;

	bool isChunked() const;

	std::map<std::string, std::string> &getHeaderFields();

	const std::string &getBody() const;

	const std::string &getQueryString() const;

	const std::string &getNormalizedPath() const;

	bool parseRequestLine(const std::string &req, size_t &pos);

	bool parseHeaders(const std::string &req, size_t &pos);

	bool parseRequestMessage(Session *sess, size_t &pos);
	bool processHeaders(Socket *sock);
//	bool parseBody(std::string &request, size_t &pos, Socket *socket);
};

#endif //UNTITLED_HTTPREQUEST_HPP
