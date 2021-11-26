//
// Created by Elayne Debi on 9/9/21.
//

#include "HttpRequest.hpp"

HttpRequest::HttpRequest() : _method(),
                             _request_uri(),
                             _query_string(),
                             _normalized_path(),
                             _http_v("HTTP/1.1"),
                             _chunked(false),
                             _header_fields(),
                             _body(),
                             _content_length(0),
                             _ready(false),
                             _max_body_size(MAX_DEFAULT_BODY_SIZE),
                             _parsing_error(HttpResponse::HTTP_OK),
                             _chunk_length(),
                             _c_bytes_left(0),
                             _skip_n(0) {}

HttpRequest::HttpRequest(
        const HttpRequest &rhs) : _method(rhs._method),
                                  _request_uri(rhs._request_uri),
                                  _query_string(rhs._query_string),
                                  _normalized_path(rhs._normalized_path),
                                  _http_v(rhs._http_v),
                                  _chunked(rhs._chunked),
                                  _header_fields(rhs._header_fields),
                                  _body(rhs._body),
//                                  _client_ip(rhs._client_ip),
                                  _content_length(rhs._content_length),
                                  _ready(rhs._ready),
                                  _max_body_size(rhs._max_body_size),
                                  _parsing_error(rhs._parsing_error),
                                  _chunk_length(rhs._chunk_length),
                                  _c_bytes_left(rhs._c_bytes_left),
                                  _skip_n(rhs._skip_n) {}

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
    _content_length  = rhs._content_length;
    _ready           = rhs._ready;
    _max_body_size   = rhs._max_body_size;
    _parsing_error   = rhs._parsing_error;
    _chunk_length    = rhs._chunk_length;
    _c_bytes_left    = rhs._c_bytes_left;
    return (*this);
}

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

bool HttpRequest::processHeaders() {
    std::map<std::string, std::string>::iterator it;
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
    if (_parsing_error != HttpResponse::HTTP_OK) {
        return (false);
    }
    if (_method == "DELETE" || _method == "GET") {
        _body.clear();
        _content_length = 0;
        _ready          = true;
        return (true);
    }
//    it           = _header_fields.find("Expect"); @todo Expect: continue add support
//    if (it != _header_fields.end()) {
//        std::cout << "found expect" << std::endl;
//        _parsing_error = HttpResponse::HTTP_CONTINUE;
//        return (true);
//    }
    return (true);
}

bool HttpRequest::setParsingError(uint16_t status) {
    _ready         = true;
    _parsing_error = status;
    return (true);
}

bool HttpRequest::headersSent(const std::string &req) {
    if (_request_uri.empty()) {
        if (req.find("\r\n\r\n") == std::string::npos) {
            if (req.size() > MAX_MESSAGE) {
                _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
                return (setParsingError(HttpResponse::HTTP_BAD_REQUEST));
            }
            return (false);
        }
        return (true);
    }
    return (false);
}

bool HttpRequest::findRouteSetResponse(Session *sess, Socket *sock) {
    VirtualServer *serv = nullptr;
    HttpResponse  *resp = nullptr;

    serv = sock->getServerByHostHeader(_header_fields);
    if (serv == nullptr) {
        _parsing_error = HttpResponse::HTTP_BAD_REQUEST;
        return (false);
    }
    try {
        resp = new HttpResponse(*sess, serv);
        sess->setResponse(resp);
        _max_body_size = resp->getMaxBodySize();
        return (true);
    }
    catch (std::exception &e) {
        _parsing_error = HttpResponse::HTTP_INTERNAL_SERVER_ERROR;
        return (false);
    }

}

/*
 * Attempt to process request. If end of request message is found, message will be processed.
 * If not, request is appended to buffer and parsing is not performed.
 */
bool HttpRequest::parseRequestMessage(Session *sess, size_t &pos, Socket *sock) {

    const std::string &req = sess->getBuffer();

    do {
        if (!headersSent(req)) break;
        if (!(parseRequestLine(req, pos))) break;
        if (!(parseHeaders(req, pos))) break;
        processUri();
        if (!(processHeaders())) break;
        if (!findRouteSetResponse(sess, sock)) break;
        if (_content_length > _max_body_size) {
            _parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
            break;
        }
    } while (false);
    if (_parsing_error != HttpResponse::HTTP_OK && _parsing_error != HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE)
        return (setParsingError(_parsing_error));
    if (_request_uri.empty())
        return (false);
    if (_content_length != 0 || isChunked())
        return (appendBody(sess, pos));
    _ready = true;
    return (true);
}



bool HttpRequest::appendBody(Session *sess, size_t &pos) {
    const std::string &buff = sess->getBuffer();
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
            _ready          = true;
            _content_length = _body.size();
            return (true);
        } else
            return (false);
    } else {
        size_t len;
        if (_parsing_error == HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE && _content_length > 0)
            _content_length -= buff.size();
        else if (buff.size() - pos + _body.size() > _content_length) {
            len = _content_length - _body.size();
            _body.insert(_body.end(), buff.begin() + pos, buff.begin() + pos + len);
        } else
            _body.insert(_body.end(), buff.begin() + pos, buff.end());
        if (_body.size() == _content_length) {
            _ready = true;
            return (true);
        }
        sess->clearBuffer();
    }
    return (false);
}

bool HttpRequest::parseChunked(Session *sess, unsigned long &pos, unsigned long bytes) {
    const std::string &request = sess->getBuffer();
//_body.reserve(1000000);
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
                    return (false);
                }
            } else if (_c_bytes_left + _body.size() > _max_body_size) {
                _parsing_error = HttpResponse::HTTP_REQUEST_ENTITY_TOO_LARGE;
            }
            _skip_n += 2;
        }
        while (_skip_n > 0 && pos < bytes) {
            ++pos;
            --_skip_n;
        }
        if (pos < bytes) {
            unsigned long end;
            if (_c_bytes_left >= bytes - pos)
                end = bytes - pos;
            else
                end = _c_bytes_left;
            if (_parsing_error == HttpResponse::HTTP_OK) {
                _body.append(request.data() + pos, end);
            }
            _c_bytes_left -= end;
            pos += end;
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

const std::string &HttpRequest::getQueryString() const {
    return _query_string;
}

const std::string &HttpRequest::getNormalizedPath() const {
    return _normalized_path;
}


bool HttpRequest::isReady() const {
    return _ready;
}


uint16_t HttpRequest::getParsingError() const {
    return _parsing_error;
}

//void HttpRequest::sendContinue(int fd) {
//    _parsing_error = HttpResponse::HTTP_OK; //@todo rewrite!
//    send(fd, "HTTP/1.1 100 Continue\r\n\r\n", 27, 0);
//}

unsigned long HttpRequest::getContentLength() const {
    return _content_length;
}

void HttpRequest::setNormalizedPath(const std::string &normalizedPath) {
    _normalized_path = normalizedPath;
}

const std::string &HttpRequest::getUriNoQuery() const {
    return _uri_no_query;
}
