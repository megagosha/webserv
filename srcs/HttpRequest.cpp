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


HttpRequest::HttpRequest() : _method(),
							 _request_uri(),
							 _query_string(),
							 _normalized_path(),
							 _http_v("HTTP/1.1"),
							 _chunked(false),
							 _header_fields(),
							 _body(),
							 _client_ip(),
							 _content_length(0),
							 _ready(false),
							 _max_body_size(MAX_DEFAULT_BODY_SIZE),
							 _parsing_error(0),
							 _chunk_length(),
							 _c_bytes_left(0)
{
};

bool
parse(const std::string &src, std::size_t &token_start, const std::string &token_delim, bool delim_exact,
	  std::size_t max_len,
	  std::string &token)
{
	token_start          = src.find_first_not_of(" \t\r\n", token_start);
	if (token_start == std::string::npos)
		return (false);
	std::size_t line_end = src.find_first_of("\r\n", token_start);
	if (line_end == std::string::npos)
		line_end = src.length();
	std::size_t token_end = src.find_first_of(token_delim, token_start);
	if (token_end == std::string::npos && delim_exact)
		return false;
	if (token_end == std::string::npos)
		token_end = line_end;
	token         = src.substr(token_start, token_end - token_start);
	if (token.empty() || token.length() > max_len)
		return (false);
	token_start = token_end;
	return (true);
}

void HttpRequest::processUri(void)
{
	_query_string = Utils::getExt(_request_uri, '?');
	_uri_no_query = Utils::getWithoutExt(_request_uri, '?');
}

bool HttpRequest::parseRequestLine(const std::string &request, size_t &pos)
{
	if (!parse(request, pos, " ", true, MAX_METHOD, _method))
	{
		_parsing_error = HttpResponse::HTTP_METHOD_NOT_ALLOWED;
		return (false);
	}
	if (!parse(request, pos, " ", true, MAX_URI, _request_uri))
	{
		_parsing_error = HttpResponse::HTTP_REQUEST_URI_TOO_LONG;
		return (false);
	}
	if (!parse(request, pos, "\r\n", false, MAX_V, _http_v) ||
		(_http_v != "HTTP/1.1" && _http_v != "HTTP/1.0"))
	{
		_parsing_error = HttpResponse::HTTP_VERSION_NOT_SUPPORTED;
		return (false);
	}
	return (true);
}

bool HttpRequest::parseHeaders(const std::string &request, size_t &pos)
{
	std::string field_name;
	std::string value;
	int         num_fields = 0;
	while (pos < request.size() && request.find("\r\n\r\n", pos) != pos)
	{
		if (num_fields > MAX_FIELDS)
		{
			_parsing_error = HttpResponse::HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE;
			return (false);
		}
		if (!parse(request, pos, ":", true, MAX_NAME, field_name))
		{
			_parsing_error = HttpResponse::HTTP_BAD_REQUEST;
			return (false);
		}
		if (!parse(request, ++pos, "\r\n", false, MAX_VALUE, value))
		{
			_parsing_error = HttpResponse::HTTP_BAD_REQUEST;
			return (false);
		}
		_header_fields.insert(std::make_pair(field_name, value));
		++num_fields;
	}
	if (request.find("\r\n\r\n", pos) == pos)
	{
		pos += 4;
		return (true);
	}
	return (false);
}

bool HttpRequest::processHeaders(Socket *sock)
{
	std::map<std::string, std::string>::iterator it;
	std::cout << "Message _body" << std::endl;
	it = _header_fields.find("Transfer-Encoding");
	//parseRequestUri();
	if (it == _header_fields.end())
	{
		it = _header_fields.find("Content-Length");
		if (it != _header_fields.end())
		{
			_content_length = std::strtoul(it->second.data(), nullptr, 10);
			if (errno == ERANGE)
			{
				_parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
				return (false);
			}

		} else if ((_method == "POST" || _method == "PUT") &&
				   _header_fields.find("Content-Length") == _header_fields.end() && !isChunked())
			_parsing_error = HttpResponse::HTTP_LENGTH_REQUIRED;
		_chunked           = false;
	} else if (it->second.find("chunked") != std::string::npos)
		_chunked = true;
	if (_parsing_error != 0)
	{
		return (false);
	}
	if (_method == "DELETE" || _method == "GET")
	{
		_body.clear();
		_content_length = 0;
		_ready          = true;
		return (false);
	}
	it           = _header_fields.find("Expect");
	if (it != _header_fields.end())
	{
		std::cout << "found expect" << std::endl;
		_parsing_error = HttpResponse::HTTP_CONTINUE;
		return (false);
	}
	it           = _header_fields.find("Host");
	VirtualServer *serv = sock->getServerByHostHeader(_header_fields);
	if (serv == nullptr)
	{
		_parsing_error = HttpResponse::HTTP_BAD_REQUEST;
		_ready         = true;
		return (false);
	}
	if (serv->getBodySizeLimit() != 0)
		_max_body_size  = MAX_DEFAULT_BODY_SIZE;
	else
		_max_body_size = serv->getBodySizeLimit();
	return (true);
}

//bool HttpRequest::parseBody(std::string &request, size_t &pos, Socket *socket)
//{
//
//	if (_chunked)
//		return (parseChunked(request, pos + 4, (long) request.size())); //@todo types fix
//	else if (request.find("\r\n\r\n", pos) == pos)
//	{
//		std::cout << "len " << _content_length << " max " << _max_body_size << std::endl;
//		if (_content_length > _max_body_size)
//		{
//			_parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
//			_ready         = true;
//			return (false);
//		}
//		_body += request.substr(pos + 4, pos + _content_length);
//		std::cout << "parsed " << "size: " << _body.size() << std::endl;
//		//        std::cout << "body content: " << _body << std::endl;
//		if (_body.size() == _content_length)
//			_ready = true;
//		else
//		{
//			std::cout << "Should continue" << std::endl;
//			_ready = true;
//			return (true);
//		}
//	} else
//		_parsing_error = HttpResponse::HTTP_BAD_REQUEST;
//	return (false);
//}

/*
 * Attempt to process request. If end of request message is found, message will be processed.
 * If not, request is appended to buffer and parsing is not performed.
 */
bool HttpRequest::parseRequestMessage(Session *sess, size_t &pos)
{

	bool              res  = false;
	const std::string &req = sess->getBuffer();
	if (_request_uri.empty())
	{
		if (req.find("\r\n\r\n") == std::string::npos)
		{
			if (req.size() > MAX_MESSAGE)
				_parsing_error = HttpResponse::HTTP_BAD_REQUEST;
			return (false);
		}
	}
	do
	{
		if (!(res = parseRequestLine(req, pos))) break;
		if (!(res = parseHeaders(req, pos))) break;
		processUri();
		if (!(res = processHeaders(sess->getServerSocket()))) break;
	} while (false);
	if (_parsing_error != 0)
	{
		_ready = true;
		return (true);
	}
	if (res && (_content_length != 0 || isChunked()))
		return (appendBody(sess, pos));
	_ready = true;
	return (res);
}

HttpRequest::HttpRequest(Session *sess, const std::string &client_ip, unsigned long bytes)
		: _client_ip(
		client_ip), _chunk_length(),
		  _c_bytes_left(0)
{

	_chunked        = false;
	_ready          = false;
	_content_length = 0;
	_parsing_error  = 0;
	size_t pos = 0;
	if (!parseRequestMessage(sess, pos))
	{
		if (_parsing_error != 0)
			return;
	}
	if (bytes == 9)
		bytes = 9;
	return;
}
////reserve field memory
//HttpRequest::HttpRequest(std::string &request, const std::string &client_ip, unsigned long bytes, Socket *sock) : _client_ip(
//        client_ip) {
//	if (sock == nullptr)
//		_chunked = false;
//    std::cout << request << std::endl;
//    _chunked = false;
//    _ready = false;
//    _content_length = 0;
//    _parsing_error = 0;

//}

bool HttpRequest::appendBody(Session *sess, size_t &pos)
{
	const std::string &buff = sess->getBuffer();
	_parsing_error = 0;
	if (pos >= buff.size())
	{
		sess->clearBuffer();
		return (false);
	}
	if (_chunked)
		return (parseChunked(sess, pos, buff.size()));
	else
	{
		_body += buff.substr(pos, _body.size() - _content_length);
		if (_body.size() == _content_length)
		{
			std::cout << "ready to send in appendBody" << std::endl;
			_ready = true;
			return (true);
		}
		std::cout << "body is not ready" << std::endl;
		sess->clearBuffer();
	}
	return (false);
}

bool HttpRequest::parseChunked(Session *sess, unsigned long &pos, unsigned long bytes)
{
	const std::string &request = sess->getBuffer();
//	std::string       unchunked;
//	unchunked.reserve(request.size() - pos);
//	std::string::size_type i      = pos;
//	std::string            size;
//	unsigned long          ch_size;
//	std::string            chunk;
//	std::string::size_type max_sz = bytes;

	while (pos < bytes)
	{
		if (_c_bytes_left == 0)
		{
			if (request.find("0\r\n\r\n", pos) == pos)
			{
				_ready = true;
				return (true);
			}
			if (request.find("\r\n") == std::string::npos)
				return (false);

			while (pos < bytes && std::isxdigit(request[pos] && _chunk_length.size() < 8))
			{
				_chunk_length += request[pos];
				++pos;
			}
			if (request[pos] != ';' && request[pos] != '\r')
			{
				_parsing_error = HttpResponse::HTTP_BAD_REQUEST;
				return (true);
			}
			pos = request.find("\r\n", pos) + 2;
			_c_bytes_left = strtoul(_chunk_length.data(), nullptr, 16);
			_chunk_length.clear();
			if (_c_bytes_left == 0)
			{
				if (errno == ERANGE)
				{
					_parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
					return (true);
				} else
				{
					_parsing_error = HttpResponse::HTTP_INTERNAL_SERVER_ERROR;
					return (true);
				}
			}

		}
		while (_c_bytes_left > 0 && bytes > pos)
		{
			_body += request[pos];
			--_c_bytes_left;
			++pos;
		}
	}
	if (pos == bytes)
	{
		sess->clearBuffer();
		_chunk_length.empty();
		return (false);
	}
	return (false);
}
//	if (_chunk == 0)
//	{
//		int ch = _session.get();
//		while (Poco::Ascii::isSpace(ch)) ch = _session.get();
//		std::string chunkLen;
//		while (Poco::Ascii::isHexDigit(ch) && chunkLen.size() < 8) { chunkLen += (char) ch; ch = _session.get(); }
//		if (ch != eof && !(Poco::Ascii::isSpace(ch) || ch == ';')) return eof;
//		while (ch != eof && ch != '\n') ch = _session.get();
//		unsigned chunk;
//		if (NumberParser::tryParseHex(chunkLen, chunk))
//			_chunk = (std::streamsize) chunk;
//		else
//			return eof;
//	}
//	if (_chunk > 0)
//	{
//		if (length > _chunk) length = _chunk;
//		int n = _session.read(buffer, length);
//		if (n > 0) _chunk -= n;
//		return n;
//	}
//	else
//	{
//		int ch = _session.get();
//		while (ch != eof && ch != '\n') ch = _session.get();
//		return 0;
//	}
//
//while (i<max_sz)
//{
//while (
//i<max_sz && std::isxdigit(request[i])
//)
//{
//size += request[i];
//++
//i;
//}
//if (i<max_sz && request[i] == ';')
//{ //skip extension
//i = request.find("\r\n", i);
//if (i == std::string::npos)
//{
//std::cout << "ERRRORRRRR " <<
//std::endl;
//_parsing_error = HttpResponse::HTTP_BAD_REQUEST;
//return (false);
//}
//}
//i += 2; // skip \r\n
//if (size.
//
//empty()
//
//)
//{
//_parsing_error = HttpResponse::HTTP_BAD_REQUEST;
//return (false);
//}
//ch_size = strtoul(size.data(), nullptr, 16);
//if (ch_size == 0)
//{
//if (errno == ERANGE)
//{
//_parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
//return (false);
//} else if (errno == 0)
//{
//_ready = true;
//return (true);
//} else
//{
//_parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
//return (false);
//}
//}
//if (_body.
//
//size()
//
//+ ch_size > _max_body_size)
//{
//_parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
//return (false);
//}
//std::cout << "SIZE " << ch_size <<
//std::endl;
//std::cout << "ch " << request[i] <<
//std::endl;
//while (i<max_sz && ch_size> 0)
//{
//_body += request[i];
//++
//i;
//--
//ch_size;
//}
//if (ch_size != 0)
//{
//_parsing_error = HttpResponse::HTTP_BAD_REQUEST;
//return (false);
//}
////        _body += chunk;
//++
//i;
//++
//i;
////        chunk.clear();
//size.
//
//clear();
//
//std::cout <<
//std::endl;
//}
//if (i == max_sz)
//{
//_ready = false; //wait for next request;
//return (false);
//}
//_parsing_error = HttpResponse::HTTP_BAD_REQUEST;
//return (false);
//}

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
};

HttpRequest::HttpRequest(
		const HttpRequest &rhs) : _method(rhs._method),
								  _request_uri(rhs._request_uri),
								  _query_string(rhs._query_string),
								  _normalized_path(rhs._normalized_path),
								  _http_v(rhs._http_v),
								  _chunked(rhs._chunked),
								  _header_fields(rhs._header_fields),
								  _body(rhs._body),
								  _client_ip(rhs._client_ip),
								  _content_length(rhs._content_length),
								  _ready(rhs._ready),
								  _max_body_size(rhs._max_body_size),
								  _parsing_error(rhs._parsing_error),
								  _chunk_length(rhs._chunk_length),
								  _c_bytes_left(rhs._c_bytes_left)
{
};

HttpRequest &HttpRequest::operator=(const HttpRequest &rhs)
{
	if (this == &rhs)
		return (*this);
	_method          = rhs._method;
	_request_uri     = rhs._request_uri;
	_query_string    = rhs._query_string;
	_normalized_path = rhs._normalized_path;
	_http_v          = rhs._http_v;
	_chunked         = rhs._chunked;
	_header_fields   = rhs._header_fields;
	_body            = rhs._body;
	_client_ip       = rhs._client_ip;
	_content_length  = rhs._content_length;
	_ready           = rhs._ready;
	_max_body_size   = rhs._max_body_size;
	_parsing_error   = rhs._parsing_error;
	_chunk_length    = rhs._chunk_length;
	_c_bytes_left    = rhs._c_bytes_left;
	return (*this);
}

const std::string &HttpRequest::getQueryString() const
{
	return _query_string;
}

const std::string &HttpRequest::getNormalizedPath() const
{
	return _normalized_path;
}

const std::string &HttpRequest::getClientIp() const
{
	return _client_ip;
}

bool HttpRequest::isReady() const
{
	if (_parsing_error != 0)
		return (true);
	return _ready;
}

void HttpRequest::setReady(bool ready)
{
	_ready = ready;
}

uint16_t HttpRequest::getParsingError() const
{
	return _parsing_error;
}

void HttpRequest::sendContinue(int fd)
{
	_parsing_error = 0;
	send(fd, "HTTP/1.1 100 Continue\r\n\r\n", 27, 0);
}

unsigned long HttpRequest::getContentLength() const
{
	return _content_length;
}

void HttpRequest::setParsingError(uint16_t parsingError)
{
	_parsing_error = parsingError;
}

void HttpRequest::setNormalizedPath(const std::string &normalizedPath)
{
	_normalized_path = normalizedPath;
}

const std::string &HttpRequest::getUriNoQuery() const
{
	return _uri_no_query;
}

