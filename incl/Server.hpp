//
// Created by Elayne Debi on 9/9/21.
//

#ifndef SERVER_HPP
#define SERVER_HPP

#include "VirtualServer.hpp"
#include "ISubscriber.hpp"
#include "IManager.hpp"
#include "Socket.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "KqueueEvents.hpp"
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include "Utils.hpp"
#include <list>
#include "Session.hpp"
#include "MimeType.hpp"

#define MAX_AWAIT_CONN 100
#define MAX_KQUEUE_EV 100

/*
 * Server {
 * Sockets int fd - Socket obj
 * Sessions int client_fd session_iterator
 * }
 *
 * Socket {
 * v_serv_map <string host_name, VirtualServer obj>
 * sessions_map <int client_fd, Session onnectionObj
 *
 * acceptConnection()
 * std::pair<int, *Session> sessions_map.insert(new_client_fd, new_client_session);
 * return (new_client_fd, this);
 *
 * processRequestFd(int) {
 * find SessionPutNewRequest();
 *
 * }
 *
 * Session {
 * HttpRequest *request
 * HttpResponse *response
 * bool response_sent
 *
 * 3 timeoutvals
 * bool keep alive;
 * }
 *
 * Pseudo
 * 1. read fd recieved from kqueue
 * 2. check Sockets for fd
 * 3. if fd found -> socket->acceptConnection();
 * 4. if fd not found search in sessions by fd;
 * 5. if found socket->proccessRequest(fd);
 */
class Socket;
class VirtualServer;
class Server : public IManager {
    typedef std::map<int, Socket>::iterator iter;
//std::map<int, Session *> _cgi_pipe_fd, session;
private:
    std::map<int, Socket>        _sockets; // <socket fd, socket obj>
    std::map<int, Session *> _sessions;
//    std::map<int, Session *> _cgi_pipes;
    std::map<int,  ISubscriber *> _subs;
    std::set<int>                _pending_sessions;
    std::list<std::string>       _tok_list;
    KqueueEvents                 _kq;

    Server();

public:

    Server &operator=(const Server &rhs);

    Server(const Server &rhs);

    //@todo create destructor
    ~Server();

    Server(const std::string &config_file);

    void addSession(std::pair<int, Session*> pair);

    virtual void subscribe(int fd, short type, ISubscriber *obj);

    virtual void unsubscribe(int fd, short type, ISubscriber *obj);

    _Noreturn virtual void loop();

//    void run(void);

    bool validate(const VirtualServer &server);

    void apply(VirtualServer &serv);

    class ServerException : public std::exception {
        const std::string m_msg;
    public:
        ServerException(const std::string &msg);

        ~ServerException() throw();

        const char *what() const throw();
    };

    void prepareResponse(std::map<int, Session *>::iterator sess_iter, long bytes);

    void acceptConnection(std::map<int, Socket>::iterator it);

    void closeConnection(int cur_fd);

//    void loopEvenets(std::pair<int, struct kevent *> &updates);
//
//    void processReadEvent(struct kevent event);
//
//    void processWriteEvent(struct kevent event);
//
//    void watchPipe(Session *sess, bool rw);
//
//    void createSession(Session *sess);
};

#endif //SERVER_HPP
