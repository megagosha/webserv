//
// Created by Elayne Debi on 9/16/21.
//

#ifndef VIRTUAL_SERVER_HPP
#define VIRTUAL_SERVER_HPP

#include <map>
#include <list>
#include <arpa/inet.h>
#include <set>
#include <vector>
#include "Utils.hpp"
#include "HttpResponse.hpp"
#include "Server.hpp"
#include "Location.hpp"

bool isNumber(const std::string &str);

class VirtualServer
{

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
	enum method_type
	{
		GET,
		POST,
		DELETE,
		OTHER
	};

   static method_type  hashMethod(std::string const &inString);

	VirtualServer();

	VirtualServer &operator=(const VirtualServer &rhs);

	VirtualServer(const VirtualServer &rhs);

	uint16_t getPort() const;

	bool validatePort() const;

	bool validateHost() const;

	bool isServerNameSet() const;

	bool validateErrorPages() const;

	bool validateLocations() const;

	in_addr_t getHost() const;

	const std::string &getServerName() const;

	bool isDefConfig() const;

	std::string getCustomErrorPagePath(short n) const;

	const std::map<short, std::string> &getErrorPages() const;

	const std::map<std::string, Location> &getLocations() const;

	unsigned long getBodySizeLimit() const;

	bool isDirectoryListingOn() const;

//	std::map<std::string, Location> location;

	void setHost(std::list<std::string>::iterator &it,
				 std::list<std::string>::iterator &end);

	void setPort(std::list<std::string>::iterator &it,
				 std::list<std::string>::iterator &end);

	void setBodySize(std::list<std::string>::iterator &it,
					 std::list<std::string>::iterator &end);

	void setErrorPage(std::list<std::string>::iterator &it,
					  std::list<std::string>::iterator &end);

	void setLocation(std::list<std::string>::iterator &it,
					 std::list<std::string>::iterator &end);

	std::map<std::string, Location>::iterator findRouteFromUri(std::string normalized_uri);

	std::map<std::string, Location>::iterator checkCgi(std::string const &path);

	//1. get request path (remove query, normalize path, find location, append root, create response)
	//2.
	HttpResponse generate(const HttpRequest &request);
    friend bool operator== (VirtualServer &lhs, VirtualServer &rhs);

	class VirtualServerException : public std::exception
	{
		const std::string m_msg;
	public:
		VirtualServerException(const std::string &msg);

		~VirtualServerException() throw();

		const char *what() const throw();
	};
};

#endif
