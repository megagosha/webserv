//
// Created by Elayne Debi on 9/9/21.
//

#ifndef UNTITLED_SERVERTEMP_HPP
#define UNTITLED_SERVERTEMP_HPP

#include "Socket.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "KqueueEvents.hpp"
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include "util.cpp"
#include <list>
#include "VirtualServer.hpp"

#define MAX_AWAIT_CONN 100
#define MAX_KQUEUE_EV 100


class Server {
    typedef std::map<int, Socket >::iterator iter;
    std::map<int, Socket> _sockets; // <socket fd, socket obj>
    std::map<int, iter> _connections; // <connection fd, socket iterator>


    std::string config_res;
    std::list<std::string> tok_list;
    std::set<std::string> config_keys;
//    std::map<int, VirtualServer> servers;
    std::map<int, HttpResponse> pending_response;
    //pair connection fd with server fd
//    std::map<int, int> connections;
    KqueueEvents kq;

    Server() {

    }

public:
    class ServerException : public std::exception {
        const std::string m_msg;
    public:
        ServerException(const std::string &msg) : m_msg(msg) {}

        ~ServerException() throw() {};

        const char *what() const throw() {
            std::cerr << "ServerError: ";
            return (m_msg.data());
        }
    };


    //@todo create destructor
    ~Server() {
        //close all connections.
        for (std::map<int, std::map<int, iter> >::iterator it = _connections.begin(); it != _connections.end(); ++it) {
            close(it->first);
        }
        for (std::map<int, Socket>::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
            it->second.clear();
        }
    }

    Server(const std::string &config_file) : kq(MAX_KQUEUE_EV) {
        std::ifstream is(config_file, std::ifstream::binary);
        std::string line;
        if ((is.rdstate() & std::ifstream::failbit) != 0)
            throw ServerException("Error opening " + config_file + "\n");
        init_keys();
        if (is) {
            size_t pos;
            std::string tok;
            tok.reserve(50);
            while (std::getline(is, line)) {
                tok.clear();
                for (std::string::iterator it = line.begin(); it != line.end(); ++it) {
                    if (std::isspace(*it) || *it == '#' || *it == ';') {
                        if (!tok.empty()) {
                            tok_list.push_back(std::string(tok));
                            tok.clear();
                        }
                        if (*it == '#')
                            break;
                        if (*it == ';') {
                            tok_list.push_back(std::string(1, *it));
                        }
                        continue;
                    }
                    tok.push_back(*it);
                }
                if (!tok.empty())
                    tok_list.push_back(std::string(tok));
            }
        } else {
            is.close();
            throw ServerException("Error EOF was not reached " + config_file + "\n");
        }
        is.close();

        std::list<std::string>::iterator end = tok_list.end();
        std::list<std::string>::iterator it = tok_list.begin();
        for (; it != end; ++it) // loop for servers
        {

            if (*it != "server" || *(++it) != "{")
                throw ServerException("Error while parsing config file");
            VirtualServer serv;
            ++it;
            std::list<std::string>::iterator check;
            while (it != end && *it != "}") //loop inside server block
            {
                if (it != end && *it == "listen") {
                    serv.setHost(it, end);
                    continue;
                }
                if (it != end && *it == "port") {
                    serv.setPort(it, end);
                    continue;
                }
                if (it != end && *it == "client_max_body_size") {
                    serv.setBodySize(it, end);
                    continue;
                }
                if (it != end && *it == "error_page") {
                    serv.setErrorPage(it, end);
                    continue;
                }
                if (it != end && *it == "location") {
                    serv.setLocation(it, end);
                    continue;
                }
                //@todo THROW ERROR if nothing else worked
            }
            validate(serv);
            apply(serv);
            ++it;
        }
        run();
    }

    void process_requests(std::pair<int, struct kevent *> &updates) {
        int i = 0;
        int new_fd = 0;
        std::map<int, VirtualServer>::iterator it;
        std::map<int, iter>::iterator ity;
        struct sockaddr s_addr;
        socklen_t s_len;

        while (i < updates.first) {
            if (updates.second[i]->filter == EVFILT_READ) {
                it = _sockets.find(updates.second[i]->ident);
                //accept new connection
                if (it != _sockets.end()) {
                    new_fd = accept(it->first, &s_addr, &s_len);
                    if (new_fd < 0) {
                        std::cout << "error accepting connection" << std::strerror(errno) << std::endl;
                        ++i;
                        continue;
                    }
                    _connections.insert(new_fd, it);
                    kq.addFd(new_fd, true); //@todo catch error
                }
                if (it == _sockets.end()) {
                    ity = _connections.find(updates.second[i]->ident);
                    if (ity == _connections.end()) {
                        std::cout << "Error in connection logic" << std::endl; //@todo delete if statement here
                        ++i;
                        continue;
                    }
                    std::string req(recv(updates.second[i]->data, updates.second[i]->ident));
                    HttpRequest request(req);
//                    typedef std::map<int, Socket >::iterator iter;
//                    std::map<int, Socket> _sockets;
//                    std::map<int, iter > _connections;
//                    HttpResponse response(request);
                    ity->second->second->generate(request);
                    pending_response.insert(std::make_pair(updates.second->ident, response))
                }
                //client closing connection
                if (updates.second[i]->flags & EV_EOF) {
                    close(updates.second[i]->ident);
                    kq.deleteFd(updates.second->ident, true); //@todo ?kqueue may clear itself on close(fd)?
                    _connections.erase(updates.second[i]->ident);
                }
            }
            i++;
        }
    }

    HttpResponse generateResponse(HttpRequest &request) {
        HttpResponse response;
        VirtualServer serv;

        std::map<std::string, std::string>::iterator rq_it;
        rq_it = request.header_fields.find("Host");
        if (rq_it == request.header_fields.end()) {
            //@todo return  400 ! implement error constructors for HttpResonse
        }
        for (std::map<int, VirtualServer>::iterator it = servers.begin(); it != servers.end(); ++it) {
            if (it->second.getServerName() == rq_it->second &&) {

            }
        }

    }

    void process_response(std::pair<int, struct kevent *> &updates) {
        int i = 0;
        std::map<int, HttpResponse>::iterator it;

        while (i < updates.first) {
            if (updates.second[i]->filter == EVFILT_WRITE) {
                it = pending_response.find(updates.second[i]->ident);
                if (it != pending_response.end()) {
                    write(it->first, it->second.getResponseString().data(), it->second.getResponseString().size());
                    pending_response.erase(it);
                    close(updates.second->ident);
                    kq.deleteFd(updates.second->ident, true);
                    _connections.erase(updates.second[i]->ident);
                }
            }
            ++i;
        }
    }

    void run(void) {
        std::pair<int, struct kevent *> updates;

        kq.init();
        for (std::map<int, VirtualServer>::iterator it = servers.begin(); it != servers.end(); ++it) {
            kq.addFd(it->first);
        }
        while (1) {
            if (exit_flag) {
                ~Server();
                return (EXIT_SUCCESS);
            }
            updates = kq.getUpdates();
            process_requests(updates);
            process_response(updates);
        }
    }

    bool validate(const VirtualServer &server) {
        if (serv.validatePort() && serv.validateHost() &&
            serv.validateErrorPages() && serv.validateLocations())
            return (true);
        throw ServerException("Config validation failed");
    }

    void apply(VirtualServer &serv) {
        for (_sockets::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
            if (serv.getHost() == it->second.getHost() && serv.getPort() == it->second.getPort()) {
                it->second.appendVirtualServer(serv);
                return;
            }
        }
        Socket sock(serv.getIp(), serv.getPort());
        sock.appendVirtualServer(serv);
        _sockets.insert(sock.getSocketFd(), sock);
    }

    void init_keys() {
        config_keys.insert("server");
        config_keys.insert("listen");
        config_keys.insert("port");
        config_keys.insert("error_page");
        config_keys.insert("client_max_body_size");
        config_keys.insert("location");
        config_keys.insert("root");
        config_keys.insert("methods");
        config_keys.insert("autoindex");
        config_keys.insert("index");
        config_keys.insert("file_upload");
        config_keys.insert("cgi_pass");
        config_keys.insert("methods");
        config_keys.insert("cgi_pass");
        config_keys.insert("return");
        config_keys.insert("{");
        config_keys.insert("}");
    }
};

#endif //UNTITLED_SERVER_HPP
