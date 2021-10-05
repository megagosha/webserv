//
// Created by Elayne Debi on 9/9/21.
//

//3 types of bodies
// fixedlength
// _chunked transfer
// no length provided

#include "HttpRequest.hpp"

std::string &leftTrim(std::string &str, std::string chars)
{
	str.erase(0, str.find_first_not_of(chars));
	return str;
}

HttpRequest::HttpRequest()
{

}

//reserve field memory
HttpRequest::HttpRequest(std::string &request)
{
	_chunked = false;
	_method.reserve(MAX_METHOD);
	_request_uri.reserve(MAX_URI);
	_http_v.reserve(MAX_V);
	leftTrim(request, "\r\n");
	//		std::istringstream iss(request);

	int i = 0;
	int end = request.length();
	//		std::string::iterator it = request.begin();
	//		std::string::iterator end = request.end();
	char ch = request[i];

	std::cout << request << std::endl;

	while (i < end && ch != ' ' && _method.length() < MAX_METHOD)
	{
		_method += ch;
		++i;
		ch = request[i];
	}
	if (_method.empty() || ch != ' ')
	{
		throw std::exception(); // _method not found || _method is not supported
	}
	++i;
	ch = request[i];
	while (i < end && _request_uri.length() < MAX_URI && ch != ' ')
	{
		_request_uri += ch;
		++i;
		ch = request[i];
	}
	if (_request_uri.empty() || ch != ' ')
		throw std::exception();
	++i;
	ch = request[i];
	while (i < end && ch != '\r' && ch != '\n' && _http_v.length() < MAX_V)
	{
		_http_v += ch;
		++i;
		ch = request[i];
	}
	if (ch != '\r' || (_http_v != "HTTP/1.1" && _http_v != "HTTP/1.0"))
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
		_header_fields.insert(std::make_pair(field_name, value));
		++num_fields;
	}
	while ((ch == '\r' || ch == '\n') && i < end)
	{
		++i;
		ch = request[i];
	}
	std::cout << "Message _body" << std::endl;
	_body.reserve(100);
	int k = 0;
	while (i < end)
	{
		_body[k] = request[i];
		++i;
		k++;
	}
}

const std::string &HttpRequest::getMethod() const
{
	return _method;
}

const std::string &HttpRequest::getRequestUri() const
{
	return _request_uri;
}

const std::string &HttpRequest::getHttpV() const
{
	return _http_v;
}

bool HttpRequest::isChunked() const
{
	return _chunked;
}

const std::map<std::string, std::string> &HttpRequest::getHeaderFields() const
{
	return _header_fields;
}

const std::string &HttpRequest::getBody() const
{
	return _body;
}

HttpRequest::HttpRequest(const HttpRequest &rhs) :
		_method(rhs._method),
		_request_uri(rhs._request_uri),
		_http_v(rhs._http_v),
		_chunked(rhs._chunked),
		_header_fields(rhs._header_fields),
		_body(rhs._body)
{}

HttpRequest &HttpRequest::operator=(const HttpRequest &rhs)
{
	_method = rhs.getMethod();
	_request_uri = rhs.getRequestUri();
	_http_v = rhs.getHttpV();
	_chunked = rhs.isChunked();
	_header_fields = rhs.getHeaderFields();
	_body = rhs.getBody();
}

