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

enum Limits
{
	MAX_METHOD            = 8,
	MAX_URI               = 2000,
	MAX_V                 = 8,
	MAX_FIELDS            = 30,
	MAX_NAME              = 100,
	MAX_VALUE             = 1000,
	MAX_DEFAULT_BODY_SIZE = 1000000
};

std::string &leftTrim(std::string &str, std::string chars);

class Socket;
class Location;
class HttpRequest
{
private:
	std::string                        _method;
	std::string                        _request_uri;
	std::string                        _query_string;
    std::string                        _uri_no_query;
public:
    const std::string &getUriNoQuery() const;

private:
    std::string                        _normalized_path;
public:
    void setNormalizedPath(const std::string &normalizedPath);

private:
    //1. removed ?query 2. expanded /../
	std::string                        _http_v; //true if http/1.1
	bool                               _chunked;
	std::map<std::string, std::string> _header_fields;
	std::string                        _body;
	std::string                        _client_ip;
	unsigned long                      _content_length;
	bool                               _ready;
	unsigned long                      _max_body_size;
	uint16_t                           _parsing_error;



public:
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
	HttpRequest(std::string &request, const std::string &client_ip, unsigned long bytes, Socket *sock);

	void parseChunked(const std::string &request, unsigned long i, long bytes);

	void appendBody(std::string &buff, long bytes);

	const std::string &getMethod() const;

	const std::string &getRequestUri() const;


	const std::string &getHttpV() const;

	bool isChunked() const;

	const std::map<std::string, std::string> &getHeaderFields() const;

	const std::string &getBody() const;

	const std::string &getQueryString() const;

	const std::string &getNormalizedPath() const;
};

#endif //UNTITLED_HTTPREQUEST_HPP
