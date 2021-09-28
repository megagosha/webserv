//
// Created by Elayne Debi on 9/9/21.
//

#ifndef UNTITLED_HTTPRESPONSE_HPP
#define UNTITLED_HTTPRESPONSE_HPP

#include <map>
#include <cstring>
#include "HttpRequest.hpp"
#include <iostream>
#include <fstream>
#include <vector>

class HttpResponse
{
public:
	std::string proto;
	std::string status_code;
	std::string status_reason;
	std::string response_string;
	std::map<std::string, std::string> header;
	std::vector<char> result;

	HttpResponse()
	{

	}

#define BUFFER_LENGTH 1000

	HttpResponse(HttpRequest &x)
	{
		proto = "HTTP/1.1";
		status_code = "200";
		status_reason = "OK";
		response_string = proto + " " + status_code + " " + status_reason + "\r\n";
		time_t now = time(nullptr);

		char buf[100];
		std::strftime(buf, 100, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&now));

		header.insert(std::make_pair("Date:", buf));
		header.insert(std::make_pair("Server:", "mg_webserv/0.01"));

		int file_len = 10000;
		result.reserve(response_string.length() + file_len + 100);
		result.insert(result.begin(), response_string.begin(), response_string.end());

		for (std::map<std::string, std::string>::iterator it = header.begin(); it != header.end(); ++it)
		{
			result.insert(result.end(), (*it).first.begin(), (*it).first.end());
			result.push_back(' ');
			result.insert(result.end(), (*it).second.begin(), (*it).second.end());
			result.push_back('\r');
			result.push_back('\n');
		}
		result.push_back('\r');
		result.push_back('\n');

		x.request_uri = "index.html";
		std::ifstream file(x.request_uri, std::ifstream::in | std::ifstream::binary);
		if (file)
		{
			file.seekg(0, file.end);
			int length = file.tellg();
			file.seekg(0, file.beg);
			result.reserve(result.size() + length);
			result.insert(result.end(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
			if (file.peek() != EOF)
				throw std::exception();
			file.close();
		}
	}

	std::vector<char> &getResponseString()
	{
		return (result);
	}
};

#endif //UNTITLED_HTTPRESPONSE_HPP
