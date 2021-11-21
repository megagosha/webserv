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
                             _max_body_size(MAX_DEFAULT_BODY_SIZE),
                             _parsing_error(0),
                             _chunk_length(),
                             _c_bytes_left(0),
                             _skip_n(0) {}


void HttpRequest::processUri(void) {
    _query_string = Utils::getExt(_request_uri, '?');
    _uri_no_query = Utils::getWithoutExt(_request_uri, '?');
}

bool HttpRequest::parseRequestLine(const std::string &request, size_t &pos) {
    if (!Utils::parse(request, pos, " ", true, MAX_METHOD, _method)) {
        _parsing_error = HttpResponse::HTTP_METHOD_NOT_ALLOWED;
        return (false);
    }
    if (!Utils::parse(request, pos, " ", true, MAX_URI, _request_uri)) {
        _parsing_error = HttpResponse::HTTP_REQUEST_URI_TOO_LONG;
        return (false);
    }
    if (!Utils::parse(request, pos, "\r\n", false, MAX_V, _http_v) ||
        (_http_v != "HTTP/1.1" && _http_v != "HTTP/1.0")) {
        _parsing_error = HttpResponse::HTTP_VERSION_NOT_SUPPORTED;
        return (false);
    }
//	_request_uri = Utils::normalizeUri(_request_uri);
    return (true);
}

bool HttpRequest::parseHeaders(const std::string &request, size_t &pos) {
    std::string field_name;
    std::string value;
    int         num_fields = 0;
    while (pos < request.size() && request.find("\r\n\r\n", pos) != pos) {
        if (num_fields > MAX_FIELDS) {
            _parsing_error = HttpResponse::HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE;
            return (false);
        }
        if (!Utils::parse(request, pos, ":", true, MAX_NAME, field_name)) {
            _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
            return (false);
        }
        if (!Utils::parse(request, ++pos, "\r\n", false, MAX_VALUE, value)) {
            _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
            return (false);
        }
        _header_fields.insert(std::make_pair(field_name, value));
        ++num_fields;
    }
    if (request.find("\r\n\r\n", pos) == pos) {
        pos += 4;
        return (true);
    }
    return (false);
}

bool HttpRequest::processHeaders(Socket *sock) {
    std::map<std::string, std::string>::iterator it;
    std::cout << "Message _body" << std::endl;
    it = _header_fields.find("Transfer-Encoding");
    //parseRequestUri();
    if (it == _header_fields.end()) {
        it = _header_fields.find("Content-Length");
        if (it != _header_fields.end()) {
            _content_length = std::strtoul(it->second.data(), nullptr, 10);
            if (errno == ERANGE) {
                _parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
                return (false);
            }

        } else if ((_method == "POST" || _method == "PUT") &&
                   _header_fields.find("Content-Length") == _header_fields.end() && !isChunked())
            _parsing_error = HttpResponse::HTTP_LENGTH_REQUIRED;
        _chunked           = false;
    } else if (it->second.find("chunked") != std::string::npos)
        _chunked = true;
    if (_parsing_error != 0) {
        return (false);
    }
    if (_method == "DELETE" || _method == "GET") {
        _body.clear();
        _content_length = 0;
        _ready          = true;
        return (false);
    }
    it           = _header_fields.find("Expect");
    if (it != _header_fields.end()) {
        std::cout << "found expect" << std::endl;
        _parsing_error = HttpResponse::HTTP_CONTINUE;
        return (false);
    }
    it           = _header_fields.find("Host");
    VirtualServer *serv = sock->getServerByHostHeader(_header_fields);
    if (serv == nullptr) {
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

/*
 * Attempt to process request. If end of request message is found, message will be processed.
 * If not, request is appended to buffer and parsing is not performed.
 */
bool HttpRequest::parseRequestMessage(Session *sess, size_t &pos) {

    bool              res  = false;
    const std::string &req = sess->getBuffer();
    if (_request_uri.empty()) {
        if (req.find("\r\n\r\n") == std::string::npos) {
            if (req.size() > MAX_MESSAGE)
                _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
            return (false);
        }
    }
    do {
        if (!(res = parseRequestLine(req, pos))) break;
        if (!(res = parseHeaders(req, pos))) break;
        processUri();
        if (!(res = processHeaders(sess->getServerSocket()))) break;
    } while (false);
    if (_parsing_error != 0) {
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
          _c_bytes_left(0), _skip_n(0) {

    _chunked        = false;
    _ready          = false;
    _content_length = 0;
    _parsing_error  = 0;
    size_t pos = 0;
    if (!parseRequestMessage(sess, pos)) {
        if (_parsing_error != 0)
            return;
    }
    if (bytes == 9)
        bytes = 9;
    return;
}

bool HttpRequest::appendBody(Session *sess, size_t &pos) {
    const std::string &buff = sess->getBuffer();
    _parsing_error = 0;
    if (pos >= buff.size()) {
        sess->clearBuffer();
        return (false);
    }
    if (_chunked) {
        if (parseChunked(sess, pos, buff.size())) {
            std::map<std::string, std::string>::iterator m_it = _header_fields.find("Transfer-Encoding");
            if (m_it != _header_fields.end() && m_it->second.find("chunked") != std::string::npos) {
                _header_fields["Content-Length"] = std::to_string(_body.size());
                _header_fields.erase(m_it);

            }
            return (true);
        } else return (false);
    } else {
        /*
         * content 25
         * body 10
         *
         * buff 300;
         *
         * buff > content_lenght
         * content_length - body.size() 25 - 10 = 15
         * 0 -> 14
         *
         */
        size_t len;
        if (buff.size() - pos + _body.size() > _content_length) {
            len = _content_length - _body.size();
            _body.insert(_body.end(), buff.begin() + pos, buff.begin() + pos + len);
        } else
            _body.insert(_body.end(), buff.begin() + pos, buff.end());
//		_body += buff.substr(pos, _body.size() - _content_length);
        if (_body.size() == _content_length) {
            std::cout << "ready to send in appendBody " << _body.size() << std::endl;
            _ready = true;
            return (true);
        }
//		std::cout << "body is not ready" << std::endl;
        sess->clearBuffer();
    }
    return (false);
}

bool HttpRequest::parseChunked(Session *sess, unsigned long &pos, unsigned long bytes) {
    const std::string &request = sess->getBuffer();
//	std::string       unchunked;
//	unchunked.reserve(request.size() - pos);
//	std::string::size_type i      = pos;
//	std::string            size;
//	unsigned long          ch_size;
//	std::string            chunk;
//	std::string::size_type max_sz = bytes;

    while (pos < bytes) {
        while (_skip_n > 0 && pos < bytes) {
            ++pos;
            --_skip_n;
        }

        if (_c_bytes_left == 0) {
            if (request.find("0\r\n\r\n", pos) == pos) {
                _ready = true;
                return (true);
            }
            if (request.find("\r\n", pos) == std::string::npos) {
                std::string &t = sess->getBuffer();
                t.erase(t.begin(), t.begin() + pos);
                return (false);
            }

            while (pos < bytes && isxdigit(request[pos]) && (_chunk_length.size() < 8)) {
                _chunk_length += request[pos];
                ++pos;
            }
            if (pos < bytes && request[pos] != ';' && request[pos] != '\r') {
                _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
                return (true);
            }
            _c_bytes_left = strtoul(_chunk_length.data(), nullptr, 16);
            _chunk_length.clear();
            if (_c_bytes_left == 0) {
                if (errno == ERANGE) {
                    _parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
                    return (true);
                } else {
                    std::string &tmp = sess->getBuffer();
                    tmp.erase(tmp.begin(), tmp.begin() + --pos);
//					_parsing_error = HttpResponse::HTTP_INTERNAL_SERVER_ERROR;
                    return (false);
                }
            }
            _skip_n += 2;
        }
        while (_skip_n > 0 && pos < bytes) {
            ++pos;
            --_skip_n;
        }
        while (_c_bytes_left > 0 && bytes > pos) {
            _body += request[pos];
            --_c_bytes_left;
            ++pos;
        }
        if (_c_bytes_left == 0)
            _skip_n += 2;

    }

    if (pos == bytes) {
        sess->clearBuffer();
        _chunk_length.empty();
        return (false);
    }
    return (false);
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

std::map<std::string, std::string> &HttpRequest::getHeaderFields() {
    return _header_fields;
}

const std::string &HttpRequest::getBody() const {
    return _body;
}

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
                                  _c_bytes_left(rhs._c_bytes_left),
                                  _skip_n(rhs._skip_n) 
{}

HttpRequest &HttpRequest::operator=(const HttpRequest &rhs) {
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
    if (_parsing_error != 0)
        return (true);
    return _ready;
}

void HttpRequest::setReady(bool ready) {
    _ready = ready;
}

uint16_t HttpRequest::getParsingError() const {
    return _parsing_error;
}

void HttpRequest::sendContinue(int fd) {
    _parsing_error = 0;
    send(fd, "HTTP/1.1 100 Continue\r\n\r\n", 27, 0);
}

unsigned long HttpRequest::getContentLength() const {
    return _content_length;
}

void HttpRequest::setParsingError(uint16_t parsingError) {
    _parsing_error = parsingError;
}

void HttpRequest::setNormalizedPath(const std::string &normalizedPath) {
    _normalized_path = normalizedPath;
}

const std::string &HttpRequest::getUriNoQuery() const {
    return _uri_no_query;
}
