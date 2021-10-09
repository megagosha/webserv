//
// Created by Elayne Debi on 9/9/21.
//

#include "HttpResponse.hpp"

void HttpResponse::setResponseString(std::string pr, std::string s_c, std::string s_r)
{
	_proto = pr;
	_status_code = s_c;
	_status_reason = s_r;
	_response_string = _proto + " " + _status_code + " " + _status_reason + "\r\n";
}


short HttpResponse::writeFileToBuffer(std::string &file_path)
{
	long long int length;
	std::ifstream file(file_path, std::ifstream::in | std::ifstream::binary);
	if (file)
	{
		file.seekg(0, file.end);
		length = file.tellg();
		if (length <= 0)
		{
			_body_size = 0;
			return (500);
		}
		file.seekg(0, file.beg);
		_body_size = (std::size_t) length;
		_body.reserve(_body.size() + length);
		_body.insert(_body.end(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		if (file.peek() != EOF)
		{
			_body_size = 0;
			return (500);
		}
		file.close();
		insertHeader("Content-Type", "text/html; charset=UTF-8"); //@todo determine content type
		return (200);
	} else
		return (404);
}

void HttpResponse::insertHeader(std::string name, std::string value)
{
	_header.insert(std::make_pair(name, value));
}

void HttpResponse::setTimeHeader(void)
{
	time_t now = time(nullptr);
	char buf[100];

	std::strftime(buf, 100, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&now));
	insertHeader("Date", buf);
	insertHeader("Server", "mg_webserv/0.01");
}

std::string HttpResponse::getReasonPhrase(short error_code)
{
	std::string reason_phrase;

	switch (error_code)
	{
		case 200 :
		{
			reason_phrase = "OK";
			break;
		}
		case 404 :
		{
			reason_phrase = "Not Found";
			break;
		}
		case 400 :
		{
			reason_phrase = "Bad Request";
			break;
		}
		case 405 :
		{
			reason_phrase = "Method not allowed";
			break;
		}
	}
	return (reason_phrase);
}

std::string HttpResponse::getErrorHtml(std::string &error, std::string &reason)
{
	std::string res;

	res += "<!DOCTYPE html>\n<html><title>" + error +
		   "</title><body><h1>" + error + " " + reason + "</h1></body></html>\n";
	return (res);
}


HttpResponse::HttpResponse()
{

}

void HttpResponse::setError(short int code, const VirtualServer &server)
{
	std::string error_code;
	std::string reason_phrase;
	std::string res;

	reason_phrase = getReasonPhrase(code);
	if (reason_phrase.empty())
	{
		reason_phrase = "Internal Server Error";
		error_code = "500";
	} else
		error_code = std::to_string(code);
	std::string path = server.getCustomErrorPagePath(code);
	setResponseString("HTTP/1.1", error_code, reason_phrase);
	if (!path.empty())
	{
		short i = writeFileToBuffer(path);
		if (i == 200)
			return;
	}
	res = getErrorHtml(error_code, reason_phrase);
	_body.insert(_body.end(), res.begin(), res.end());
	_body_size = _body.size();
}

//error response constructor
HttpResponse::HttpResponse(short n, const VirtualServer &server)
{
	setError(n, server);
}

HttpResponse::HttpResponse(const HttpRequest &request, std::string &request_uri, VirtualServer &server,
						   Location &loc)
{
	short err;

	if (!loc.methodAllowed(request.getMethod()))
	{
		insertHeader("Allow", "GET, DELETE, POST");
		setError(405, server);
		return;
	}
	if (request.getMethod() == "GET")
	{
		err = writeFileToBuffer(request_uri);
		if (err == 200)
		{
			setResponseString("HTTP/1.1", "200", "OK");
			return;
		} else
		{
			setError(err, server);
			return;
		}
	} else
		setError(900, server);
}


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

int HttpResponse::sendResponse(int fd)
{
	std::map<std::string, std::string>::iterator it = _header.begin();
	std::vector<char> headers_vec;

	setTimeHeader();
	if (_body_size > 0)
		insertHeader("Content-Length", std::to_string(_body.size()));
	headers_vec.reserve(500);
	headers_vec.insert(headers_vec.end(), _response_string.begin(), _response_string.end());
	for (it = _header.begin(); it != _header.end(); ++it)
	{
		headers_vec.insert(headers_vec.end(), it->first.begin(), it->first.end());
		headers_vec.push_back(':');
		headers_vec.push_back(' ');
		headers_vec.insert(headers_vec.end(), it->second.begin(), it->second.end());
		headers_vec.push_back('\r');
		headers_vec.push_back('\n');
	}
	headers_vec.push_back('\r');
	headers_vec.push_back('\n');

	send(fd, headers_vec.data(), headers_vec.size(), 0);
	if (_body_size > 0)
		send(fd, _body.data(), _body.size(), 0);
	return (EXIT_SUCCESS);
}

HttpResponse::HttpResponse(const HttpResponse &rhs) :
		_proto(rhs._proto),
		_status_code(rhs._status_code),
		_status_reason(rhs._status_reason),
		_response_string(rhs._response_string),
		_header(rhs._header),
		_body(rhs._body),
		_body_size(rhs._body_size)
{};

HttpResponse &HttpResponse::operator=(const HttpResponse &rhs)
{
	if (this == &rhs)
		return (*this);
	_proto = rhs.getProto();
	_status_code = rhs.getStatusCode();
	_status_reason = rhs.getStatusReason();
	_response_string = rhs.getResponseString();
	_header = rhs.getHeader();
	_body = rhs.getBody();
	_body_size = rhs.getBodySize();
	return (*this);
}

const std::string &HttpResponse::getProto() const
{
	return _proto;
}

const std::string &HttpResponse::getStatusCode() const
{
	return _status_code;
}

const std::string &HttpResponse::getStatusReason() const
{
	return _status_reason;
}

const std::string &HttpResponse::getResponseString() const
{
	return _response_string;
}

const std::map<std::string, std::string> &HttpResponse::getHeader() const
{
	return _header;
}

const std::vector<char> &HttpResponse::getBody() const
{
	return _body;
}

size_t HttpResponse::getBodySize() const
{
	return _body_size;
}


