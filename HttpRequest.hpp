//
// Created by Elayne Debi on 9/9/21.
//

//3 types of bodies
// fixedlength
// chunked transfer
// no length provided

#ifndef UNTITLED_HTTPREQUEST_HPP
#define UNTITLED_HTTPREQUEST_HPP

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

std::string &leftTrim(std::string &str, std::string chars)
{
	str.erase(0, str.find_first_not_of(chars));
	return str;
}

class HttpRequest
{
public:
	std::string method;
	std::string request_uri;
	std::string http_v; //true if http/1.1
	bool chunked;
	std::map<std::string, std::string> header_fields;
	std::string body;

	HttpRequest()
	{

	}

	//reserve field memory
	HttpRequest(std::string &request, std::set<std::string> &methods)
	{
		chunked = false;
		method.reserve(MAX_METHOD);
		request_uri.reserve(MAX_URI);
		http_v.reserve(MAX_V);
		leftTrim(request, "\r\n");
		//		std::istringstream iss(request);

		int i = 0;
		int end = request.length();
//		std::string::iterator it = request.begin();
//		std::string::iterator end = request.end();
		char ch = request[i];

		std::cout << request << std::endl;

		while (i < end && ch != ' ' && method.length() < MAX_METHOD)
		{
			method += ch;
			++i;
			ch = request[i];
		}
		if (method.empty() || methods.find(method) == methods.end() || ch != ' ')
		{
			throw std::exception(); // method not found || method is not supported
		}
		++i;
		ch = request[i];
		while (i < end && request_uri.length() < MAX_URI && ch != ' ')
		{
			request_uri += ch;
			++i;
			ch = request[i];
		}
		if (request_uri.empty() || ch != ' ')
			throw std::exception();
		++i;
		ch = request[i];
		while (i < end && ch != '\r' && ch != '\n' && http_v.length() < MAX_V)
		{
			http_v += ch;
			++i;
			ch = request[i];
		}
		if (ch != '\r' || (http_v != "HTTP/1.1" && http_v != "HTTP/1.0"))
			throw std::exception();
		while (i < end && (ch == '\n' || ch == '\r'))
		{
			++i;
			ch = request[i];
		}
		//parse headers
		std::string field_name;
		field_name.reserve(50);
		std::string value;
		value.reserve(50);
		int num_fields = 0;
		while (i < end && ch != '\r' && ch != '\n')
		{
			ch = request[i];
			if (num_fields > MAX_FIELDS)
				throw std::exception();
			field_name.clear();
			value.clear();
			while (ch != '\n' && ch != ':' && i < end && field_name.length() < MAX_NAME)
			{
				field_name += ch;
				++i;
				ch = request[i];
			}
			if (ch != ':')
				throw std::exception();
			while ((ch == ':' || ch == ' ') && i < end)
			{
				++i;
				ch = request[i];
			}
			while (i < end && std::isspace(ch) && ch != '\n' && ch != '\r')
			{
				++i;
				ch = request[i];
			}
			while (i < end && ch != '\n' && ch != '\r' && value.length() < MAX_VALUE)
			{
				value += ch;
				++i;
				ch = request[i];
			}
			while ((ch == '\r' || ch == '\n') && i < end)
			{
				++i;
				ch = request[i];
			}
			while (ch == ' ' || ch == '\t')
			{
				while (i < end && ch != '\r' && ch != '\n' && value.length() < MAX_VALUE)
				{
					value += ch;
					++i;
					ch = request[i];
				}
				if (ch == '\r' && i < end)
				{
					++i;
					ch = request[i];
				}
				if (ch == '\n' && i < end)
				{
					++i;
					ch = request[i];
				} else if (ch != EOF)
					throw std::exception();
			}
			header_fields.insert(std::make_pair(field_name, value));
			++num_fields;
		}
		while ((ch == '\r' || ch == '\n') && i < end)
		{
			++i;
			ch = request[i];
		}
		std::cout << "Message body" << std::endl;
		body.reserve(100);
		int k = 0;
		while (i < end)
		{
			body[k] = request[i];
			++i;
			k++;
		}
	};
};

#endif //UNTITLED_HTTPREQUEST_HPP
