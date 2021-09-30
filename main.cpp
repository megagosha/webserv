# include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <csignal>
#include "KqueueEvents.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
//@todo process errors in func
//@todo create objects/classes
//@todo http parser
//@todo error pages
//@todo directory listing pages
// GET, POST, and DELETE at least
// server listens on multiple ports
// choose host
// servername
// uploading files
// provide default error pages && directory listings
// use only 1 poll (select, kqueue, epoll)
// check macros  FD_SET, FD_CLR, FD_ISSET, FD_ZERO



//Questions
//kevent changes should i create new kevent structure for each event or one is enough for all cases?

std::string recv(int bytes, int socket)
{
	std::string output(bytes, 0);
	if (read(socket, &output[0], bytes) < 0)
	{
		std::cerr << "Failed to read data from socket.\n";
	}
	return output;
}

volatile std::sig_atomic_t gSignalStatus;
volatile bool exit_flag = false;

void sig_pipe_handler(int signal)
{
	gSignalStatus = signal;
	std::cout << "signal " << signal << std::endl;
	if (signal != SIGPIPE)
		exit_flag = true;
}

int main(void)
{
	std::signal(SIGPIPE, sig_pipe_handler);
	std::signal(SIGABRT, sig_pipe_handler);
	std::signal(SIGQUIT, sig_pipe_handler);
	std::signal(SIGINT, sig_pipe_handler);

	int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0)
	{
		std::cout << "Error: socket " << std::strerror(errno) << std::endl;
		return (EXIT_FAILURE);
	}
	fcntl(fd, F_SETFL, O_NONBLOCK);
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(5000); //port number;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(addr.sin_zero, 0, 8);
	addr.sin_len = sizeof(addr);

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	{
		std::cout << "Error: bind " << std::strerror(errno) << std::endl;
		close(fd);
		return (EXIT_FAILURE);
	}
	int backlog = 20; //get from system defined macros
	if (listen(fd, backlog) == -1)
	{
		std::cout << "Error: listen " << std::strerror(errno) << std::endl;
		close(fd);
		return (EXIT_FAILURE);
	}

	KqueueEvents kq(10, fd);

	socklen_t s_len;
	struct sockaddr s_addr;
	int s_new;
	std::pair<int, struct kevent *> updates;
	std::set<std::string> methods;
	std::map<int, HttpResponse> pending_response;
	methods.insert(std::string("GET"));
	methods.insert(std::string("POST"));
	methods.insert(std::string("DELETE"));
	while (1)
	{
		if (exit_flag)
		{
			std::cout << "closing gracefully" << std::endl;
			close(fd);
			exit(EXIT_SUCCESS);
		}
		updates = kq.getUpdates();
		//New connection
		if (updates.first == 1 && updates.second->ident == fd)
		{
			std::cout << "New connection" << std::endl;
			s_new = accept(fd, &s_addr, &s_len); // @todo add error handling
			if (s_new < 0)
			{
				std::cout << "Error: accept " << std::strerror(errno) << std::endl;
				return (EXIT_FAILURE);
			}
			kq.addFd(s_new);
		}
		//client closing connection to
		if (updates.first != fd && updates.second->flags & EV_EOF && updates.second->filter == EVFILT_READ)
		{
			std::cout << "closing socket " << updates.second->ident << std::endl;
			close(updates.second->ident);
			kq.deleteFd(updates.second->ident);
		}
			//received request
		else if (updates.second->ident != fd && updates.second->filter == EVFILT_READ)
		{
			std::string req(recv(updates.second->data, updates.second->ident));
			std::cout << "Recieved bytes: " << updates.second->data << std::endl;
			try
			{
				HttpRequest x;
				HttpResponse y;
				x = HttpRequest(req, methods);
				y = HttpResponse(x);
				pending_response.insert(std::make_pair(updates.second->ident, y));
			}
			catch (std::exception &e)
			{
				std::cout << e.what() << std::endl;
				close(fd);
			}
//			std::cout << recv(updates.second->data, updates.second->ident) << std::endl;
//			char *hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
//			write(updates.second->ident, hello, strlen(hello));
		} else if (updates.second->ident != fd && updates.second->filter == EVFILT_WRITE)
		{
			std::map<int, HttpResponse>::iterator it = pending_response.find(updates.second->ident);
			if (it != pending_response.end())
			{
				write(updates.second->ident, (*it).second.getResponseString().data(),
					  (*it).second.getResponseString().size());
				pending_response.erase(it);
				close(updates.second->ident);
				kq.deleteFd(updates.second->ident);
			}
		}
	}

	std::cout << "Hello, World!" << std::endl;
	std::cout << "new socket " << fd << std::endl;
	close(fd);
	return (EXIT_SUCCESS);
}
