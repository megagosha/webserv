//
// Created by Elayne Debi on 9/9/21.
//

#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "VirtualServer.hpp"
#include "HttpRequest.hpp"
#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstddef>

//
//class VirtualServer
//{
//public:
//	class Location;
//};
class VirtualServer;

class Location;

class HttpResponse
{
private:
	std::string _proto;
	std::string _status_code;
	std::string _status_reason;
	std::string _response_string;
	std::map<std::string, std::string> _header;
	std::vector<char> _body;
	std::size_t _body_size;

	HttpResponse();

	void setResponseString(std::string pr, std::string s_c, std::string s_r);

	short writeFileToBuffer(std::string &file_path);

	void insertHeader(std::string name, std::string value);

	void setTimeHeader(void);

	std::string getErrorHtml(std::string &error, std::string &reason);

	void setError(short int code, const VirtualServer &server);

public:
	static std::string getReasonPhrase(short error_code);

	//error response constructor
	HttpResponse(short n, const VirtualServer &server);

	HttpResponse(const HttpRequest &request, std::string &request_uri, VirtualServer &server, Location &loc);

	HttpResponse(const HttpResponse &rhs);

	HttpResponse &operator=(const HttpResponse &rhs);

	int sendResponse(int fd);

	const std::string &getProto() const;

	const std::string &getStatusCode() const;

	const std::string &getStatusReason() const;

	const std::string &getResponseString() const;

	const std::map<std::string, std::string> &getHeader() const;

	const std::vector<char> &getBody() const;

	size_t getBodySize() const;
};
//	HttpResponse(HttpRequest &x)
//	{
//		_proto = "HTTP/1.1";
//		_status_code = "200";
//		_status_reason = "OK";
//		_response_string = _proto + " " + _status_code + " " + _status_reason + "\r\n";
//
//
//		int file_len = 10000;
//		result.reserve(_response_string.length() + file_len + 100);
//		result.insert(result.begin(), _response_string.begin(), _response_string.end());
//
//		for (std::map<std::string, std::string>::iterator it = _header.begin(); it != _header.end(); ++it)
//		{
//			result.insert(result.end(), (*it).first.begin(), (*it).first.end());
//			result.push_back(' ');
//			result.insert(result.end(), (*it).second.begin(), (*it).second.end());
//			result.push_back('\r');
//			result.push_back('\n');
//		}
//		result.push_back('\r');
//		result.push_back('\n');
//
//		x._request_uri = "index.html";
//		std::ifstream file(x._request_uri, std::ifstream::in | std::ifstream::binary);
//		if (file)
//		{
//			file.seekg(0, file.end);
//			int length = file.tellg();
//			file.seekg(0, file.beg);
//			result.reserve(result.size() + length);
//			result.insert(result.end(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
//			if (file.peek() != EOF)
//				throw std::exception();
//			file.close();
//		}
//	}

#endif