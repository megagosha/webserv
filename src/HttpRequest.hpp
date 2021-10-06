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

enum Limits
{
	MAX_METHOD = 8,
	MAX_URI = 2000,
	MAX_V = 8,
	MAX_FIELDS = 30,
	MAX_NAME = 100,
	MAX_VALUE = 1000
};

std::string &leftTrim(std::string &str, std::string chars);

class HttpRequest
{
private:
	std::string							_method;
	std::string 						_request_uri;
	std::string 						_http_v; //true if http/1.1
	bool								_chunked;
	std::map<std::string, std::string> _header_fields;
	std::string							_body;
public:
	HttpRequest();

	HttpRequest(const HttpRequest &rhs);

	HttpRequest &operator=(const HttpRequest &rhs);

	//reserve field memory
	HttpRequest(std::string &request);

	const std::string &getMethod() const;

	const std::string &getRequestUri() const;

	const std::string &getHttpV() const;

	bool isChunked() const;

	const std::map<std::string, std::string> &getHeaderFields() const;

	const std::string &getBody() const;
};

#endif //UNTITLED_HTTPREQUEST_HPP
