//
// Created by Elayne Debi on 9/9/21.
//

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <set>
#include <sstream>
#include <map>
#include <iostream>
#include "Utils.hpp"
#include "HttpResponse.hpp"
#include "Socket.hpp"
#include "Location.hpp"
#include "Session.hpp"

class Socket;

class Location;

class Session;

class HttpRequest {
private:
    std::string                        _method;
    std::string                        _request_uri;
    std::string                        _query_string;
    std::string                        _uri_no_query;
    std::string                        _normalized_path;
    std::string                        _http_v; //true if http/1.1
    bool                               _chunked;
    std::map<std::string, std::string> _header_fields;
    std::string                        _body;
    unsigned long                      _content_length;
    bool                               _ready;
    unsigned long                      _max_body_size;
    uint16_t                           _parsing_error;
    std::string                        _chunk_length;
    unsigned long                      _c_bytes_left;
    short                              _skip_n;

public:

    enum Limits {
        MAX_METHOD            = 8,
        MAX_URI               = 2000,
        MAX_V                 = 100,
        MAX_FIELDS            = 100,
        MAX_NAME              = 100,
        MAX_VALUE             = 1000,
        MAX_MESSAGE           = 10000,
        MAX_DEFAULT_BODY_SIZE = 1000000
    };

    void setNormalizedPath(const std::string &normalizedPath);

    const std::string &getUriNoQuery() const;

    void processUri(void);

    unsigned long getContentLength() const;


    bool isReady() const;

    uint16_t getParsingError() const;

    HttpRequest();

    HttpRequest(const HttpRequest &rhs);

    HttpRequest &operator=(const HttpRequest &rhs);

    bool parseRequestLine(const std::string &req, size_t &pos);

    bool parseHeaders(const std::string &req, size_t &pos);

    bool parseRequestMessage(Session *sess, size_t &pos, Socket *serv);

    bool processHeaders();

    bool parseChunked(Session *sess, unsigned long &pos, unsigned long bytes);

    bool appendBody(Session *sess, size_t &pos);

    bool findRouteSetResponse(Session *sess, Socket *sock);

    const std::string &getMethod() const;

    const std::string &getRequestUri() const;

    bool setParsingError(uint16_t status);

    const std::string &getHttpV() const;

    bool headersSent(const std::string &req);

    bool isChunked() const;

    std::map<std::string, std::string> &getHeaderFields();

    const std::string &getBody() const;

    const std::string &getQueryString() const;

    const std::string &getNormalizedPath() const;

    //    void sendContinue(int fd);

//	bool parseBody(std::string &request, size_t &pos, Socket *socket);
};

#endif //HTTP_REQUEST_HPP
