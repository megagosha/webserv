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
                             _parsing_error(0) {};

std::pair<bool, std::size_t>
parse(std::string &src, std::size_t token_start, const std::string &token_delim, bool delim_exact, std::size_t max_len,
      std::string &token) {
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
HttpRequest::HttpRequest(std::string &request, const std::string &client_ip, unsigned long bytes) : _client_ip(
        client_ip) {
    _chunked = false;
    _ready = false;
    _content_length = 0;
    _parsing_error = 0;
    std::pair<bool, std::size_t> rdt;
    if (!(rdt = parse(request, 0, " ", true, MAX_METHOD, _method)).first) {
        _parsing_error = HttpResponse::HTTP_METHOD_NOT_ALLOWED;
        return;
    }
    if (!(rdt = parse(request, rdt.second, " ", true, MAX_URI, _request_uri)).first) {
        _parsing_error = HttpResponse::HTTP_REQUEST_URI_TOO_LONG;
        return;
    }
    if (!(rdt = parse(request, rdt.second, "\r\n", false, MAX_V, _http_v)).first ||
        (_http_v != "HTTP/1.1" && _http_v != "HTTP/1.0")) {
        _parsing_error = HttpResponse::HTTP_VERSION_NOT_SUPPORTED;
        return;
    }
    //parse headers
    std::string field_name;
    std::string value;
    int num_fields = 0;
    while (rdt.second < bytes && request.find("\r\n\r\n", rdt.second) != rdt.second) {
        if (num_fields > MAX_FIELDS) {
            _parsing_error = HttpResponse::HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE;
            return;
        }
        if (!(rdt = parse(request, rdt.second, ":", true, MAX_NAME, field_name)).first) {
            _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
        }
        if (!(rdt = parse(request, rdt.second + 1, "\r\n", false, MAX_VALUE, value)).first) {
            _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
            return;
        }
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
        // 	} e_pair(field_name, value));
        _header_fields.insert(std::make_pair(field_name, value));
        ++num_fields;
    }
    std::map<std::string, std::string>::iterator it;
    std::cout << "Message _body" << std::endl;
    it = _header_fields.find("Transfer-Encoding");
    parse_request_uri();
    if (it == _header_fields.end()) {

        it = _header_fields.find("Content-Length");
        if (it != _header_fields.end()) {
            if (it->second.size() > 10) {
                _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
            } else {
                _content_length = std::strtoul(it->second.data(), nullptr, 10);
                if (errno == ERANGE) {
                    _parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
                }
            }
        } else if (_method == "POST" && _header_fields.find("Content-Length") == _header_fields.end() && !isChunked())
            _parsing_error = HttpResponse::HTTP_LENGTH_REQUIRED;
        _chunked = false;
    } else if (it->second.find("chunked") != std::string::npos)
        _chunked = true;
    else
        _chunked = false;
    if (_parsing_error != 0) {
        _ready = true;
        return;
    }
    if (!_chunked && _content_length == 0 && _method == "POST") {
        _parsing_error = HttpResponse::HTTP_LENGTH_REQUIRED;
        _ready = true;
        return;
    }
    it = _header_fields.find("Expect");
    if (it != _header_fields.end()) {
        std::cout << "found expect" << std::endl;
        _parsing_error = HttpResponse::HTTP_CONTINUE;
        return;
    }

    if (_chunked) {
        parseChunked(request, rdt.second, (long) bytes); //@todo types fix
        return;
    } else if (request.find("\r\n\r\n", rdt.second) == rdt.second) {
        _body += request.substr(rdt.second + 4, rdt.second + _content_length);
        std::cout << "parsed " << "size: " << _body.size() << std::endl;
        std::cout << "body content: " << _body << std::endl;
        if (_body.size() == _content_length)
            _ready = true;
        else {
            std::cout << "Should continue" << std::endl;
            return;
        }
    }
    else
        _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
}

void HttpRequest::appendBody(std::string &buff, long bytes) {
    std::cout << "body appended " << std::endl;
    if (_chunked)
        parseChunked(buff, 0, bytes);
    else {
        _body += buff.substr(0, _body.size() - _content_length);
        if (_body.size() == _content_length) {
            std::cout << "ready to send in appendBody" << std::endl;
            _ready = true;
            return;
        }
        std::cout << "body is not ready" << std::endl;
    }
}

void HttpRequest::parseChunked(const std::string &request, unsigned long pos, long bytes) {
    std::string unchunked;
    unchunked.reserve(request.size() - pos);
    std::string::size_type i = pos;
    std::string size;
    unsigned long ch_size;
    std::string chunk;
    std::string::size_type max_sz = bytes;
    while (i < max_sz) {
        while (i < max_sz && std::isxdigit(request[i])) {
            size += request[i];
            ++i;
        }
        if (i < max_sz && request[i] == ';') { //skip extension
            i = request.find("\r\n", i);
            if (i == std::string::npos) {
                std::cout << "ERRRORRRRR " <<
                          std::endl;
            }
        }
        i += 2; // skip \r\n
        if (size.empty()) {
            _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
            return;
        }
        ch_size = strtoul(size.data(), nullptr, 16);
        if (ch_size == 0) {
            if (errno == ERANGE) {
                _parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
                return;
            } else if (errno == 0) {
                _ready = true;
                return;
            } else {
                _parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
                return;
            }
        }
        std::cout << "SIZE " << ch_size << std::endl;
        std::cout << "ch " << request[i] << std::endl;
        while (i < max_sz && ch_size > 0) {
            chunk += request[i];
            ++i;
            --ch_size;
        }
        if (ch_size != 0) {
            _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
            return;
        }
        _body += chunk;
        ++i;
        ++i;
        chunk.clear();
        size.clear();
        std::cout << std::endl;
    }
    if (i == max_sz) {
        _ready = false; //wait for next request;
        return;
    }
    _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
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
};

HttpRequest::HttpRequest(const HttpRequest &rhs) : _method(rhs._method),
                                                   _request_uri(rhs._request_uri),
                                                   _query_string(rhs._query_string),
                                                   _normalized_path(rhs._normalized_path),
                                                   _http_v(rhs._http_v),
                                                   _chunked(rhs._chunked),
                                                   _header_fields(rhs._header_fields),
                                                   _body(rhs._body),
                                                   _client_ip(rhs._client_ip),
                                                   _ready(rhs._ready) {};

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
    _ready = rhs._ready;
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

bool HttpRequest::isReady() const {
    return _ready;
}

void HttpRequest::setReady(bool ready) {
    _ready = ready;
}

uint16_t HttpRequest::getParsingError() const {
    return _parsing_error;
}

void HttpRequest::setParsingError(uint16_t parsingError) {
    _parsing_error = parsingError;
}
