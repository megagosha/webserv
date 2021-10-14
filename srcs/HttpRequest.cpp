//
// Created by Elayne Debi on 9/9/21.
//

//3 types of bodies
// fixedlength
// _chunked transfer
// no length provided

#include "HttpRequest.hpp"

std::string &leftTrim(std::string &str, std::string chars) {
    str.erase(0, str.find_first_not_of(chars));
    return str;
}


HttpRequest::HttpRequest() {}

std::pair<bool, std::size_t> parse(std::string &src, std::size_t token_start, const std::string &token_delim, bool delim_exact, std::size_t max_len, std::string &token)
{
	token_start = src.find_first_not_of(" \t\r\n", token_start);
	if (token_start == std::string::npos)
		return std::make_pair(false, 0);
	std::size_t line_end = src.find_first_of("\r\n", token_start);
	if (line_end == std::string::npos)
		line_end = src.length();
	std::size_t token_end = src.find_first_of(token_delim, token_start);
	if (token_end == std::string::npos && delim_exact)
		return std::make_pair(false, 0);
	if (token_end == std::string::npos)
		token_end = line_end;
	token = src.substr(token_start, token_end - token_start);
	if (token.empty() || token.length() > max_len)
		return std::make_pair(false, 0);
	return std::make_pair(true, token_end);
}

void HttpRequest::parse_request_uri(void) {

    _query_string = Utils::getExt(_request_uri, '?');

    std::string no_query = Utils::getWithoutExt(_request_uri, '?');
    _normalized_path = Utils::normalizePath(no_query);
}

//reserve field memory
HttpRequest::HttpRequest(std::string &request, const std::string& client_ip) : _client_ip(client_ip) {
{
	_chunked = false;
	std::cout << request << std::endl;

	std::pair<bool, std::size_t> rdt;
	if (!(rdt = parse(request, 0, " ", true, MAX_METHOD, _method)).first)
		throw std::exception();
	if (!(rdt = parse(request, rdt.second, " ", true, MAX_URI, _request_uri)).first)
		throw std::exception();
	if (!(rdt = parse(request, rdt.second, "\r\n", false, MAX_V, _http_v)).first || (_http_v != "HTTP/1.1" && _http_v != "HTTP/1.0"))
		throw std::exception();
	//parse headers
	std::string field_name;
	std::string value;
	int num_fields = 0;
	while (rdt.second < request.length() && request.find("\r\n\r\n", rdt.second) != rdt.second)
	{
		if (num_fields > MAX_FIELDS)
			throw std::exception();
		if (!(rdt = parse(request, rdt.second, ":", true, MAX_NAME, field_name)).first)
			throw std::exception();
		if (!(rdt = parse(request, rdt.second + 1, "\r\n", false, MAX_VALUE, value)).first)
			throw std::exception();
		// rdt.second += 2;
		// while (rdt.second < request.length() && request[rdt.second] == ' ' || request[rdt.second] == '\t')
		// 	while (ch == ' ' || ch == '\t')
		// 	{
		// 		while (i < end && ch != '\r' && ch != '\n' && value.length() < MAX_VALUE)
		// 		{
		// 			value += ch;
		// 			++i;
		// 			ch = request[i];
		// 		}
		// 		if (ch == '\r' && i < end)
		// 		{
		// 			++i;
		// 			ch = request[i];
		// 		}
		// 		if (ch == '\n' && i < end)
		// 		{
		// 			++i;
		// 			ch = request[i];
		// 		}
		// 		else if (ch != EOF)
		// 			throw std::exception();
		// 	}
		_header_fields.insert(std::make_pair(field_name, value));
		++num_fields;
	}
	std::cout << "Message _body" << std::endl;
	rdt.second = request.find_first_not_of("\r\n", rdt.second);
	if (rdt.second != std::string::npos)
		_body = request.substr(rdt.second);
  parse_request_uri();
}

const std::string &HttpRequest::getMethod() const {
    return _method;
}

const std::string &HttpRequest::getRequestUri() const {
    return _request_uri;
}

const std::string &HttpRequest::getHttpV() const {
    return _http_v;
}

bool HttpRequest::isChunked() const {
    return _chunked;
}

const std::map<std::string, std::string> &HttpRequest::getHeaderFields() const {
    return _header_fields;
}

const std::string &HttpRequest::getBody() const {
    return _body;
}

HttpRequest::HttpRequest(const HttpRequest &rhs) :
        _method(rhs._method),
        _request_uri(rhs._request_uri),
        _query_string(rhs._query_string),
        _normalized_path(rhs._normalized_path),
        _http_v(rhs._http_v),
        _chunked(rhs._chunked),
        _header_fields(rhs._header_fields),
        _body(rhs._body),
        _client_ip(rhs._client_ip){}

HttpRequest &HttpRequest::operator=(const HttpRequest &rhs) {
    if (this == &rhs)
        return (*this);
    _method = rhs._method;
    _request_uri = rhs._request_uri;
    _query_string = rhs._query_string;
    _normalized_path = rhs._normalized_path;
    _http_v = rhs._http_v;
    _chunked = rhs._chunked;
    _header_fields = rhs._header_fields;
    _body = rhs._body;
    _client_ip = rhs._client_ip;
    return (*this);
}

const std::string &HttpRequest::getQueryString() const {
    return _query_string;
}

const std::string &HttpRequest::getNormalizedPath() const {
    return _normalized_path;
}

const std::string &HttpRequest::getClientIp() const {
    return _client_ip;
}
