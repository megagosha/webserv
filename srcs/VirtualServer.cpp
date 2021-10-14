//
// Created by Elayne Debi on 9/16/21.
//

#include "VirtualServer.hpp"

bool isNumber(const std::string &str)
{
	for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
	{
		if (std::isdigit(*it) == 0) return false;
	}
	return true;
}


uint16_t VirtualServer::getPort() const
{
	return _port;
}

bool VirtualServer::validatePort() const
{
	std::cout << _port << std::endl;
	if (_port < 1 || _port > 65535)
		return (false);
	return (true);
}

bool VirtualServer::validateHost() const
{
	if (_host == INADDR_NONE)
		return (false);
	return (true);
}

bool VirtualServer::isServerNameSet() const
{
	return (!_server_name.empty());
}

bool VirtualServer::validateErrorPages() const
{
	for (std::map<short, std::string>::const_iterator it = _error_pages.begin(); it != _error_pages.end(); ++it)
	{
		if ((*it).first < 100 || (*it).first > 599)
			return (false);
		if (!Utils::fileExistsAndReadable((*it).second))
			return (false);
	}
	return (true);
}

bool VirtualServer::validateLocations() const
{
	std::string ret;
	for (std::map<std::string, Location>::const_iterator it = _locations.begin(); it != _locations.end(); ++it)
	{
		if (it->first[0] == '*')
		{
			ret = it->second.getCgiPass();
			if (ret.empty() || !Utils::fileExistsAndExecutable(ret.c_str()))
				return (false);
			continue;
		}
		if (it->first[0] != '/')
			return (false);
	}
	return (true);
}

in_addr_t VirtualServer::getHost() const
{
	return _host;
}

const std::string &VirtualServer::getServerName() const
{
	return _server_name;
}

bool VirtualServer::isDefConfig() const
{
	return _def_config;
}


std::string VirtualServer::getCustomErrorPagePath(short n) const
{
	std::map<short, std::string>::const_iterator it;

	it = _error_pages.find(n);
	if (it == _error_pages.end())
		return ("");
	else
		return (it->second);
}

const std::map<short, std::string> &VirtualServer::getErrorPages() const
{
	return _error_pages;
}

const std::map<std::string, Location> &VirtualServer::getLocations() const
{
	return _locations;
}

unsigned long VirtualServer::getBodySizeLimit() const
{
	return _body_size_limit;
}

bool VirtualServer::isDirectoryListingOn() const
{
	return _directory_listing_on;
}

VirtualServer::VirtualServer()
{
}

void VirtualServer::setHost(std::list<std::string>::iterator &it,
							std::list<std::string>::iterator &end)
{
	Utils::skipTokens(it, end, 1);
	_host = inet_addr((*it).data());
	Utils::skipTokens(it, end, 2);
}

void VirtualServer::setPort(std::list<std::string>::iterator &it,
							std::list<std::string>::iterator &end)
{
	unsigned int port = 0;

	Utils::skipTokens(it, end, 1);
	port = std::stoi((*it));
	_port = (uint16_t) port;
	Utils::skipTokens(it, end, 2);
}

void VirtualServer::setBodySize(std::list<std::string>::iterator &it,
								std::list<std::string>::iterator &end)
{
	unsigned long body_size = 0;

	Utils::skipTokens(it, end, 1);
	body_size = std::stoi(*it);
	_body_size_limit = body_size;
	Utils::skipTokens(it, end, 2);
}

void VirtualServer::setErrorPage(std::list<std::string>::iterator &it,
								 std::list<std::string>::iterator &end)
{
	short err;
	std::string path;

	Utils::skipTokens(it, end, 1);
	err = (short) std::stoi(*it);
	Utils::skipTokens(it, end, 1);
	_error_pages.insert(std::make_pair(err, *it));
	Utils::skipTokens(it, end, 2);
}

void VirtualServer::setLocation(std::list<std::string>::iterator &it,
								std::list<std::string>::iterator &end)
{
	Location loc;
	std::string path;

	Utils::skipTokens(it, end, 1);
	path = *it;
	loc.setLocation(it, end, path);
	_locations.insert(std::make_pair(path, loc));
}

std::map<std::string, Location>::iterator VirtualServer::checkCgi(std::string const &path)
{
	size_t find = path.find_last_of('.');
	if (find == std::string::npos)
		return (_locations.end());
	std::string ext = path.substr(find);
	std::string loc_ext;
	if (ext.empty())
		return (_locations.end());
	for (std::map<std::string, Location>::iterator it = _locations.begin(); it != _locations.end(); ++it)
	{
		find = it->first.find_last_of('.');
		if (find != std::string::npos)
			loc_ext = it->first.substr(it->first.find_last_of('.'));
		else
			loc_ext = "";
		if (loc_ext == ext)
			return (it);
	}
	return (_locations.end());
}

std::map<std::string, Location>::iterator VirtualServer::findRouteFromUri(std::string normalized_uri)
{
	if (normalized_uri.empty())
		return (_locations.end());

	unsigned long i = 0;
	int best = 0;
	int cur_best = 0;
	std::map<std::string, Location>::iterator it;
	it = checkCgi(normalized_uri);
	if (it == _locations.end())
		it = _locations.begin();
	else
		return (it);
	std::map<std::string, Location>::iterator search_res;
	for (; it != _locations.end(); ++it)
	{
		i = 0;
		++cur_best;
		std::string cur_string = it->first;
		while (i < it->first.size())
		{
			if (i >= normalized_uri.size() || cur_string[i] != normalized_uri[i])
				break;
			if (cur_string[i] == '/')
				++cur_best;
			++i;
		}
		if (i == it->first.size())
		{
			if (i == normalized_uri.size())
			{
				best = cur_best;
				search_res = it;
				break;
			}
			if (cur_best > best)
			{
				search_res = it;
				best = cur_best;
			}
		}
	}

	if (best == 0)
		return (_locations.end());
	return (search_res);
}

//1. get request path (remove query, normalize path, find location, append root, create response)
//2.
HttpResponse VirtualServer::generate(const HttpRequest &request)
{
	std::string req_no_query;
	std::map<std::string, Location>::iterator it;
	std::map<std::string, Location>::iterator loc_route;

	req_no_query = request.getNormalizedPath();

	if (req_no_query.empty())
		return (HttpResponse(400, *this));

	loc_route = findRouteFromUri(req_no_query); //@todo add search for cgi_location
	if (loc_route == _locations.end())
		return (HttpResponse(400, *this));

	if (!loc_route->second.methodAllowed(request.getMethod()))
	{
		//https://datatracker.ietf.org/doc/html/rfc7231#section-6.5.5
		//@todo TEST Allow header field with supported methods;
		return (HttpResponse(405, *this, loc_route->second));
	}
	//@todo validate root field in each location
	req_no_query = req_no_query.replace(0, 1, loc_route->second.getRoot());
	if (*request.getRequestUri().rbegin() == '/')
	{
		std::string index = loc_route->second.getIndex();
		if (!index.empty())
		{
			req_no_query = req_no_query + index;
			return (HttpResponse(request, req_no_query, *this, loc_route->second));
		} else if (loc_route->second.isAutoindexOn() && request.getMethod() == "GET")
			//@todo create directory listing page
			return (HttpResponse(500, *this));
		//index enabled
		return (HttpResponse(400, *this));
	} else
	{
		return (HttpResponse(request, req_no_query, *this, loc_route->second));
	}
}

VirtualServer &VirtualServer::operator=(const VirtualServer &rhs)
{
	if (this == &rhs)
		return (*this);
	_port = rhs._port;
	_host = rhs._host;
	_server_name = rhs._server_name;
	_def_config = rhs._def_config;
	_error_pages = rhs._error_pages;
	_locations = rhs._locations;
	_body_size_limit = rhs._body_size_limit;
	_directory_listing_on = rhs._directory_listing_on;
	return (*this);
}

bool operator==(VirtualServer &lhs, VirtualServer &rhs)
{
	if (&lhs == &rhs)
		return (true);
	if (
			lhs._port != rhs._port ||
			lhs._host != rhs._host ||
			lhs._server_name != rhs._server_name ||
			lhs._def_config != rhs._def_config ||
			lhs._error_pages != rhs._error_pages ||
			lhs._body_size_limit != rhs._body_size_limit ||
			lhs._directory_listing_on != rhs._directory_listing_on)
		return (false);
	return (true);
}


VirtualServer::VirtualServer(const VirtualServer &rhs) :
		_port(rhs._port),
		_host(rhs._host),
		_server_name(rhs._server_name),
		_def_config(rhs._def_config),
		_error_pages(rhs._error_pages),
		_locations(rhs._locations),
		_body_size_limit(rhs._body_size_limit),
		_directory_listing_on(rhs._directory_listing_on)
{}

//check if _method is allowed
//check if request is to file or folder
// if folder check if autoindex is enabled
// check if

//process request

VirtualServer::VirtualServerException::VirtualServerException(const std::string &msg) : m_msg(msg)
{}

VirtualServer::VirtualServerException::~VirtualServerException() throw()
{};

const char *VirtualServer::VirtualServerException::what() const throw()
{
	std::cerr << "Server config error: ";
	return (m_msg.data());
}



