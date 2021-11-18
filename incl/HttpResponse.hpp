//
// Created by Elayne Debi on 9/9/21.
//

#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "VirtualServer.hpp"
#include "HttpRequest.hpp"
#include "MimeType.hpp"
#include "KqueueEvents.hpp"
#include <map>
#include <iostream>
#include<fstream>
#include "CgiHandler.hpp"
#include "ISubscriber.hpp"
#include <fstream>
#include <vector>
#include <string>
#include <cstddef>
#include <Socket.hpp>
#include "Session.hpp"
#include "FileStats.hpp"
#include <sys/types.h>
#include <sys/wait.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

//
//class VirtualServer
//{
//public:
//	class Location;
//};
class VirtualServer;

class Location;

class Socket;

class CgiHandler;

class HttpResponse : public ISubscriber {
public:
    static const std::string AUTOINDEX_HTML;
    enum HTTPStatus {
        HTTP_CONTINUE                        = 100,
        HTTP_SWITCHING_PROTOCOLS             = 101,
        HTTP_PROCESSING                      = 102,
        HTTP_OK                              = 200,
        HTTP_CREATED                         = 201,
        HTTP_ACCEPTED                        = 202,
        HTTP_NONAUTHORITATIVE                = 203,
        HTTP_NO_CONTENT                      = 204,
        HTTP_RESET_CONTENT                   = 205,
        HTTP_PARTIAL_CONTENT                 = 206,
        HTTP_MULTI_STATUS                    = 207,
        HTTP_ALREADY_REPORTED                = 208,
        HTTP_IM_USED                         = 226,
        HTTP_MULTIPLE_CHOICES                = 300,
        HTTP_MOVED_PERMANENTLY               = 301,
        HTTP_FOUND                           = 302,
        HTTP_SEE_OTHER                       = 303,
        HTTP_NOT_MODIFIED                    = 304,
        HTTP_USE_PROXY                       = 305,
        HTTP_USEPROXY                        = 305, /// @deprecated
        // UNUSED: 306
        HTTP_TEMPORARY_REDIRECT              = 307,
        HTTP_PERMANENT_REDIRECT              = 308,
        HTTP_BAD_REQUEST                     = 400,
        HTTP_UNAUTHORIZED                    = 401,
        HTTP_PAYMENT_REQUIRED                = 402,
        HTTP_FORBIDDEN                       = 403,
        HTTP_NOT_FOUND                       = 404,
        HTTP_METHOD_NOT_ALLOWED              = 405,
        HTTP_NOT_ACCEPTABLE                  = 406,
        HTTP_PROXY_AUTHENTICATION_REQUIRED   = 407,
        HTTP_REQUEST_TIMEOUT                 = 408,
        HTTP_CONFLICT                        = 409,
        HTTP_GONE                            = 410,
        HTTP_LENGTH_REQUIRED                 = 411,
        HTTP_PRECONDITION_FAILED             = 412,
        HTTP_REQUEST_ENTITY_TOO_LARGE        = 413,
        HTTP_REQUESTENTITYTOOLARGE           = 413, /// @deprecated
        HTTP_REQUEST_URI_TOO_LONG            = 414,
        HTTP_REQUESTURITOOLONG               = 414, /// @deprecated
        HTTP_UNSUPPORTED_MEDIA_TYPE          = 415,
        HTTP_UNSUPPORTEDMEDIATYPE            = 415, /// @deprecated
        HTTP_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
        HTTP_EXPECTATION_FAILED              = 417,
        HTTP_IM_A_TEAPOT                     = 418,
        HTTP_ENCHANCE_YOUR_CALM              = 420,
        HTTP_MISDIRECTED_REQUEST             = 421,
        HTTP_UNPROCESSABLE_ENTITY            = 422,
        HTTP_LOCKED                          = 423,
        HTTP_FAILED_DEPENDENCY               = 424,
        HTTP_UPGRADE_REQUIRED                = 426,
        HTTP_PRECONDITION_REQUIRED           = 428,
        HTTP_TOO_MANY_REQUESTS               = 429,
        HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
        HTTP_UNAVAILABLE_FOR_LEGAL_REASONS   = 451,
        HTTP_INTERNAL_SERVER_ERROR           = 500,
        HTTP_NOT_IMPLEMENTED                 = 501,
        HTTP_BAD_GATEWAY                     = 502,
        HTTP_SERVICE_UNAVAILABLE             = 503,
        HTTP_GATEWAY_TIMEOUT                 = 504,
        HTTP_VERSION_NOT_SUPPORTED           = 505,
        HTTP_VARIANT_ALSO_NEGOTIATES         = 506,
        HTTP_INSUFFICIENT_STORAGE            = 507,
        HTTP_LOOP_DETECTED                   = 508,
        HTTP_NOT_EXTENDED                    = 510,
        HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511
    };
private:
    std::string                        _proto;
    u_int16_t                          _status_code;
    std::string                        _status_reason;
    std::string                        _response_string;
    std::string                        _absolute_path;
//	const VirtualServer *_serv;
    std::map<std::string, std::string> _response_headers;
    std::string                        _body;
    std::size_t                        _body_size;
    size_t                             _pos;
    CgiHandler                         *_cgi;
    std::vector<char>                  _headers_vec;

public:
    CgiHandler *getCgi() const;
//    std::map<std::string, std::string> _cgi_env;
//    std::string                        _cgi_path; //@todo should be recieved through session or other method which has access to location obj.
//    pid_t            _cgi_num;

    //	bool _cgi;
//	CgiHandler *_cgi_obj;

public:
    void processEvent(int fd, size_t bytes_available, int16_t filter, bool eof, Server *serv);

    void setResponseString(const std::string &pr, HTTPStatus status);

    HTTPStatus writeFileToBuffer(const std::string &file_path);

    void insertHeader(std::string name, std::string value);

    void setTimeHeader(void);

    static std::string getErrorHtml(std::string &error, std::string &reason);

    void setError(HTTPStatus code, const VirtualServer *server);

    HttpResponse(HTTPStatus code, const VirtualServer *server); //error page constructor

    HttpResponse();

    bool writeToCgi(HttpRequest *req, size_t bytes);

    bool readCgi(size_t bytes, bool eof);

//	HttpResponse(const HttpRequest &request, const Socket *sock, const VirtualServer *ptr);


    const std::string &getAbsolutePath() const;

//	const VirtualServer *getServ() const;

//	const Location *getLoc() const;

    virtual ~HttpResponse();

//	bool isCgi() const;

    static const std::string &getReasonForStatus(HTTPStatus status);

    HttpResponse(
            const HttpResponse &rhs);

    HttpResponse &operator=(const HttpResponse &rhs);

    int sendResponse(int fd, HttpRequest *req, size_t bytes);

    const std::string &getProto() const;

    uint16_t getStatusCode() const;

    const std::string &getStatusReason() const;

    const std::string &getResponseString() const;

    const std::map<std::string, std::string> &getHeader() const;

    const std::string &getBody() const;

//    void prepareCgiEnv(HttpRequest &request, const std::string &absolute_path, const uint16_t serv_port);

    size_t getBodySize() const;

    HTTPStatus executeCgi(HttpRequest *req);

    HttpResponse(Session &session,
                 const VirtualServer *config);

    void processGetRequest(const VirtualServer *serv, const Location *loc, HttpRequest *req);

    void processPostRequest(const VirtualServer *serv);

    void processPutRequest(const VirtualServer *serv, const Location *loc, HttpRequest *req);

    void
    processDeleteRequest(const VirtualServer *pServer, HttpRequest *req);

    bool ParseCgiHeaders(size_t end);

    void prepareData();

    void insertTableIntoBody(const std::string &str, const std::string &uri);

    void getAutoIndex(const std::string &path, const std::string &uri_path);

    static const std::string HTTP_REASON_CONTINUE;
    static const std::string HTTP_REASON_SWITCHING_PROTOCOLS;
    static const std::string HTTP_REASON_PROCESSING;
    static const std::string HTTP_REASON_OK;
    static const std::string HTTP_REASON_CREATED;
    static const std::string HTTP_REASON_ACCEPTED;
    static const std::string HTTP_REASON_NONAUTHORITATIVE;
    static const std::string HTTP_REASON_NO_CONTENT;
    static const std::string HTTP_REASON_RESET_CONTENT;
    static const std::string HTTP_REASON_PARTIAL_CONTENT;
    static const std::string HTTP_REASON_MULTI_STATUS;
    static const std::string HTTP_REASON_ALREADY_REPORTED;
    static const std::string HTTP_REASON_IM_USED;
    static const std::string HTTP_REASON_MULTIPLE_CHOICES;
    static const std::string HTTP_REASON_MOVED_PERMANENTLY;
    static const std::string HTTP_REASON_FOUND;
    static const std::string HTTP_REASON_SEE_OTHER;
    static const std::string HTTP_REASON_NOT_MODIFIED;
    static const std::string HTTP_REASON_USE_PROXY;
    static const std::string HTTP_REASON_TEMPORARY_REDIRECT;
    static const std::string HTTP_REASON_PERMANENT_REDIRECT;
    static const std::string HTTP_REASON_BAD_REQUEST;
    static const std::string HTTP_REASON_UNAUTHORIZED;
    static const std::string HTTP_REASON_PAYMENT_REQUIRED;
    static const std::string HTTP_REASON_FORBIDDEN;
    static const std::string HTTP_REASON_NOT_FOUND;
    static const std::string HTTP_REASON_METHOD_NOT_ALLOWED;
    static const std::string HTTP_REASON_NOT_ACCEPTABLE;
    static const std::string HTTP_REASON_PROXY_AUTHENTICATION_REQUIRED;
    static const std::string HTTP_REASON_REQUEST_TIMEOUT;
    static const std::string HTTP_REASON_CONFLICT;
    static const std::string HTTP_REASON_GONE;
    static const std::string HTTP_REASON_LENGTH_REQUIRED;
    static const std::string HTTP_REASON_PRECONDITION_FAILED;
    static const std::string HTTP_REASON_REQUEST_ENTITY_TOO_LARGE;
    static const std::string HTTP_REASON_REQUEST_URI_TOO_LONG;
    static const std::string HTTP_REASON_UNSUPPORTED_MEDIA_TYPE;
    static const std::string HTTP_REASON_REQUESTED_RANGE_NOT_SATISFIABLE;
    static const std::string HTTP_REASON_EXPECTATION_FAILED;
    static const std::string HTTP_REASON_IM_A_TEAPOT;
    static const std::string HTTP_REASON_ENCHANCE_YOUR_CALM;
    static const std::string HTTP_REASON_MISDIRECTED_REQUEST;
    static const std::string HTTP_REASON_UNPROCESSABLE_ENTITY;
    static const std::string HTTP_REASON_LOCKED;
    static const std::string HTTP_REASON_FAILED_DEPENDENCY;
    static const std::string HTTP_REASON_UPGRADE_REQUIRED;
    static const std::string HTTP_REASON_PRECONDITION_REQUIRED;
    static const std::string HTTP_REASON_TOO_MANY_REQUESTS;
    static const std::string HTTP_REASON_REQUEST_HEADER_FIELDS_TOO_LARGE;
    static const std::string HTTP_REASON_UNAVAILABLE_FOR_LEGAL_REASONS;
    static const std::string HTTP_REASON_INTERNAL_SERVER_ERROR;
    static const std::string HTTP_REASON_NOT_IMPLEMENTED;
    static const std::string HTTP_REASON_BAD_GATEWAY;
    static const std::string HTTP_REASON_SERVICE_UNAVAILABLE;
    static const std::string HTTP_REASON_GATEWAY_TIMEOUT;
    static const std::string HTTP_REASON_VERSION_NOT_SUPPORTED;
    static const std::string HTTP_REASON_VARIANT_ALSO_NEGOTIATES;
    static const std::string HTTP_REASON_INSUFFICIENT_STORAGE;
    static const std::string HTTP_REASON_LOOP_DETECTED;
    static const std::string HTTP_REASON_NOT_EXTENDED;
    static const std::string HTTP_REASON_NETWORK_AUTHENTICATION_REQUIRED;
    static const std::string HTTP_REASON_UNKNOWN;
    static const std::string DATE;
    static const std::string SET_COOKIE;
};

#endif
