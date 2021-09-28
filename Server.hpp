//
// Created by Elayne Debi on 9/9/21.
//

#ifndef UNTITLED_SERVER_HPP
#define UNTITLED_SERVER_HPP

#include "Socket.h"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "KqueueEvents.hpp"
#include <map>
#include <string>
#include <fstream>
#include <iostream>

#include <list>
#include "VirtualServer.hpp"

#define MAX_AWAIT_CONN 100;
#define MAX_KQUEUE_EV 100;

class Server {

    std::string config_res;
    std::list<std::string> tok_list;
    std::set<std::string> config_keys;
    std::map<int, VirtualServer> servers;
    std::map<int, HttpResponse> pending_response;
    //pair connection fd with server fd
    std::map<int, int> connections;
    KqueueEvents kq;


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

    Server() {

    }

    //@todo create destructor
    ~Server() {
        for (std::map<int, VirtualServer>::iterator it = servers.begin(); it != servers.end(); ++it) {
            close(it->first);
        }
    }

    Server(const std::string &config_file) {
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

    void process_requests(std::pair<int, struct kevent *> &updates)
    {
        int i = 0;
        int new_fd = 0;
        std::map<int, VirtualServer>::iterator it;
        struct sockaddr s_addr;
        socklen_t s_len;

        while (i < updates.first)
        {
            it = servers.find(updates.second[i]->ident);
            if (it != servers.end())
            {
                new_fd = accept(it->first, &s_addr, &s_len);
                if (new_fd < 0)
                {
                    std::cout << "error accepting connection" << std::strerror(errno) << std::endl;
                    ++i;
                    continue;
                }
                connections.insert(new_fd, it->first);
                kq.addFd(new_fd, true);
            }
            if (it == servers.end() && updates.second[i]->filter == EVFILT_READ)
            {
                std::string req(recv(updates.second->data, updates.second[i]->ident));
                HttpRequest request(req);
                HttpResponse response(request);
                pending_response.insert(std::make_pair(updates.second->ident, response))
            }
            i++;
        }

    }

    void process_response(std::pair<int, struct kevent *> &updates)
    {

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
        sockaddr_in addr;
        int fd;

        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd < 0)
            throw ServerException("Failed opening socket: " + std::strerror(errno));
        fcntl(fd, F_SETFL, O_NONBLOCK);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(serv.getPort()); //port number;
        addr.sin_addr.s_addr = serv.getHost();
        memset(addr.sin_zero, 0, 8);
        addr.sin_len = sizeof(addr);
        if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
            close(fd);
            throw ServerException("Failed to bind socket " << std::strerror(errno));
        }
        if (listen(fd, MAX_AWAIT_CONN) == -1) {
            close(fd);
            throw ServerException("Failed to listen on socket " << std::strerror(errno));
        }
        servers.insert(std::make_pair(fd, serv));
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
