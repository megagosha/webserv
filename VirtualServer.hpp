//
// Created by Elayne Debi on 9/16/21.
//

#ifndef UNTITLED_VIRTUALSERVER_HPP
#define UNTITLED_VIRTUALSERVER_HPP

#include "Server.hpp"
#include <string>
#include <map>
#include <list>
#include <arpa/inet.h>
#include <set>
#include <vector>
#include "utils.cpp"

bool isNumber(const std::string &str) {
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (std::isdigit(*it) == 0) return false;
    }
    return true;
}

class Server;

class VirtualServer {
public:
    enum method_type {
        GET,
        POST,
        DELETE,
        OTHER
    };

    method_type static hash_method(std::string const &inString) {
        if (inString == "GET") return GET;
        if (inString == "POST") return POST;
        if (inString == "DELETE") return DELETE;
        return (OTHER);
    }

    class Location {
    private:
        bool _autoindex_on;
        std::string _file_upload;
        std::string _index;
        std::string _root;
        std::string _cgi_pass;
        std::vector<bool> _methods;//0 -> get 1 -> post 2 -> delete
        std::string _ret;
    public:
        bool isAutoindexOn() const {
            return _autoindex_on;
        }

        bool isFileUploadOn() const {
            return !_file_upload.empty();
        }

        const std::string &getIndex() const {
            return _index;
        }

        const std::string &getRoot() const {
            return _root;
        }

        const std::string &getCgiPass() const {
            return _cgi_pass;
        }

        const std::vector<bool> &getMethods() const {
            return _methods;
        }

        const std::string &getRet() const {
            return _ret;
        }

    public:
        Location() : _autoindex_on(true),
                     _file_upload(""),
                     _index("index.html"), _root("/"), _cgi_pass(""), _ret("") {
            _methods.insert(_methods.begin(), 3, true);
        }

        Location(const Location &rhs) : _autoindex_on(rhs._autoindex_on),
                                        _file_upload(rhs._file_upload),
                                        _index(rhs._index), _root(rhs._root), _cgi_pass(""), _ret(rhs._ret) {
            _methods = rhs._methods;
        }

        Location &operator=(const Location &rhs) {
            if (this != &rhs) {
                _autoindex_on = rhs._autoindex_on;
                _file_upload = rhs._file_upload;
                _index = rhs._index;
                _root = rhs._root;
                _ret = rhs._ret;
                _cgi_pass = rhs._cgi_pass;
                _methods = rhs._methods;
            }
            return *this;
        }

        ~Location() {
        }

        void setLocation(std::list<std::string>::iterator &it,
                         std::list<std::string>::iterator &end, std::string path) {

            skip_tok(it, end, 2);

            if (path[0] == '*' && *it == "cgi_pass") {
                _cgi_pass = *it;
                skip_tok(it, end, 2);
//                location.insert(std::make_pair(path, loc));
                skip_tok(it, end, 1);
                return;
            }
            std::list<std::string>::iterator check;
            while (it != end && *it != "}") {
                check = it;
                if (*it == "root" && (++it) != end) {
                    _root = *it;
                    skip_tok(it, end, 2);
                }
                if (*it == "methods") {
                    skip_tok(it, end, 1);

                    _methods[0] = false;
                    _methods[1] = false;
                    _methods[2] = false;
                    while (*it != ";") {
                        switch (VirtualServer::hash_method(*it)) {
                            case GET:
                                _methods[0] = true;
                                break;
                            case POST:
                                _methods[1] = true;
                                break;
                            case DELETE:
                                _methods[2] = true;
                                break;
                            default:
                                throw VirtualServerException("Method not supported " + *it);
                        }
                        skip_tok(it, end, 1);
                    }
                    skip_tok(it, end, 1);
                }
                if (*it == "autoindex") {
                    skip_tok(it, end, 1);
                    if (*it == "on" || *it == "off")
                        _autoindex_on = *it == "on";
                    else
                        throw VirtualServerException("Autoindex error: ;");
                    skip_tok(it, end, 2);
                }
                if (*it == "return") {
                    skip_tok(it, end, 1);
                    _ret = *it;
                    skip_tok(it, end, 2);
                }
                if (*it == "index") {
                    skip_tok(it, end, 1);
                    _index = *it;
                    skip_tok(it, end, 2);
                }
                if (*it == "file_upload") {
                    skip_tok(it, end, 1);
                    _file_upload = *it;
                    skip_tok(it, end, 2);
                }
                if (it != end && *it == "}")
                    break;
                if (it == check)
                    throw VirtualServerException("Error location parsing");
            }
            if (it == end || *it != "}")
                throw VirtualServerException("Syntax error: }");
            skip_tok(it, end, 1);
        }
    };

private:
    uint16_t _port;
    in_addr_t _host;
    std::string _server_name;
    bool _def_config; //@todo delete field
    std::map<short, std::string> _error_pages;
    std::map<std::string, Location> _locations;
    unsigned long _body_size_limit;
    bool _directory_listing_on;
public:
    uint16_t getPort() const {
        return _port;
    }

    bool validatePort() const {
        if (_port < 1 || _port > 65535)
            return (false);
        return (true);
    }

    bool validateHost() const {
        if (_host == INADDR_NONE)
            return (false);
        return (true);
    }

    bool isServerNameSet() const {
        return (!_server_name.empty());
    }

    bool validateErrorPages() const {
        for (std::map<short, std::string>::const_iterator it = _error_pages.begin(); it != _error_pages.end(); ++it) {
            if ((*it).first < 100 || (*it).first > 599)
                return (false);
            if (!fileExistsAndReadable((*it).second))
                return (false);
        }
        return (true);
    }

    bool validateLocations() const {
        std::string ret;
        for (std::map<std::string, Location>::const_iterator it = _locations.begin(); it != location.end(); ++it) {
            if (it->first[0] == '*') {
                ret = it->second.getRet();
                if (ret.empty() || !fileExistsAndExecutable(ret.c_str()))
                    return (false);
                continue;
            }
            if (it->first[0] != '/')
                return (false);
        }
        return (true);
    }

    in_addr_t getHost() const {
        return _host;
    }

    const std::string &getServerName() const {
        return _server_name;
    }

    bool isDefConfig() const {
        return _def_config;
    }

    const std::map<short, std::string> &getErrorPages() const {
        return _error_pages;
    }

    const std::map<std::string, Location> &getLocations() const {
        return _locations;
    }

    unsigned long getBodySizeLimit() const {
        return _body_size_limit;
    }

    bool isDirectoryListingOn() const {
        return _directory_listing_on;
    }

public:
    std::map<std::string, Location> location;

    VirtualServer() {
    }

    static void skip_tok(std::list<std::string>::iterator &it,
                         std::list<std::string>::iterator &end, int num) {
        for (int i = 0; i < num; ++i) {
            if (it == end)
                throw VirtualServerException("Token syntax error");
            ++it;
        }
        if (it == end)
            throw VirtualServerException("Token syntax error");
    }

    void setHost(std::list<std::string>::iterator &it,
                 std::list<std::string>::iterator &end) {
        skip_tok(it, end, 1);
        _host = inet_addr((*it).data());
        skip_tok(it, end, 2);
    }

    void setPort(std::list<std::string>::iterator &it,
                 std::list<std::string>::iterator &end) {
        unsigned int port = 0;

        skip_tok(it, end, 1);
        port = std::stoi((*it));
        _port = (uint16_t) port;
        skip_tok(it, end, 2);
    }

    void setBodySize(std::list<std::string>::iterator &it,
                     std::list<std::string>::iterator &end) {
        unsigned long body_size = 0;

        skip_tok(it, end, 1);
        body_size = std::stoi(*it);
        _body_size_limit = body_size;
        skip_tok(it, end, 2);
    }

    void setErrorPage(std::list<std::string>::iterator &it,
                      std::list<std::string>::iterator &end) {
        short err;
        std::string path;

        skip_tok(it, end, 1);
        err = (short) std::stoi(*it);
        skip_tok(it, end, 1);
        _error_pages.insert(std::make_pair(err, *it));
        skip_tok(it, end, 2);
    }

    void setLocation(std::list<std::string>::iterator &it,
                     std::list<std::string>::iterator &end) {
        Location loc;
        std::string path;

        skip_tok(it, end, 1);
        path = *it;
        loc.setLocation(it, end, path);
        location.insert(std::make_pair(path, loc));
    }

    HttpResponse generate(const HttpRequest &request)
    {

    }

    class VirtualServerException : public std::exception {
        const std::string m_msg;
    public:
        VirtualServerException(const std::string &msg) : m_msg(msg) {}

        ~VirtualServerException() throw() {};

        const char *what() const throw() {
            std::cerr << "Server config error: ";
            return (m_msg.data());
        }
    };

};

#endif //UNTITLED_VIRTUALSERVER_HPP
