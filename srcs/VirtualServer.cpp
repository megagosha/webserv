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
	std::string                                          ret;
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
		if (it->second.isFileUploadOn() && it->second.getRoot().empty())
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

VirtualServer::VirtualServer() : _server_name("")
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
	port  = std::stoi((*it));
	_port = (uint16_t) port;
	Utils::skipTokens(it, end, 2);
}

void VirtualServer::setBodySize(std::list<std::string>::iterator &it,
								std::list<std::string>::iterator &end)
{
	unsigned long body_size = 0;

	Utils::skipTokens(it, end, 1);
	body_size        = std::stoi(*it) * 1000000; //value is given in mb
	_body_size_limit = body_size;
	Utils::skipTokens(it, end, 2);
}

void VirtualServer::setServerName(std::list<std::string>::iterator &it,
								  std::list<std::string>::iterator &end)
{
	Utils::skipTokens(it, end, 1);
	_server_name = *it; //value is given in mb
	Utils::skipTokens(it, end, 2);
}

void VirtualServer::setErrorPage(std::list<std::string>::iterator &it,
								 std::list<std::string>::iterator &end)
{
	short       err;
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
	Location    loc;
	std::string path;

	Utils::skipTokens(it, end, 1);
	path = *it;
	loc.setLocation(it, end, path);
	_locations.insert(std::make_pair(path, loc));
}

std::map<std::string, Location>::const_iterator VirtualServer::checkCgi(std::string const &path) const
{
	size_t find = path.find_last_of('.');
	if (find == std::string::npos)
		return (_locations.end());
	std::string ext = path.substr(find);
	std::string loc_ext;
	if (ext.empty())
		return (_locations.end());
	for (std::map<std::string, Location>::const_iterator it = _locations.begin(); it != _locations.end(); ++it)
	{
		if (it->second.getCgiPass().empty())
			continue;
		find        = it->first.find_last_of('.');
		if (find != std::string::npos)
			loc_ext = it->first.substr(it->first.find_last_of('.'));
		else
			loc_ext = "";
		if (loc_ext == ext)
			return (it);
	}
	return (_locations.end());
}


/*
 *  if loc is not included in path -> no match
 * full_match path == loc return immediately
 * other count size; choose result in the end
 */

std::map<std::string, Location>::const_iterator VirtualServer::findRouteFromUri(const std::string &normalized_uri) const
{
	std::map<std::string, Location>::const_iterator it;
	std::map<std::string, Location>::const_iterator tmp  = _locations.end();
	size_t                                          best = 0;

	if (normalized_uri.empty())
		return (_locations.end());
	it     = _locations.begin();
	for (; it != _locations.end(); ++it)
	{
		if (it->first == normalized_uri || it->first == (normalized_uri + '/'))
			return (it);
		size_t pos = normalized_uri.find(it->first);
		if (pos != 0)
			continue;
		else
		{
			best = it->first.size();
			tmp  = it;
		}
	}
	return (tmp);
}

VirtualServer &VirtualServer::operator=(const VirtualServer &rhs)
{
	if (this == &rhs)
		return (*this);
	_port                 = rhs._port;
	_host                 = rhs._host;
	_server_name          = rhs._server_name;
	_def_config           = rhs._def_config;
	_error_pages          = rhs._error_pages;
	_locations            = rhs._locations;
	_body_size_limit      = rhs._body_size_limit;
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

std::string getFullPath(const std::string &loc_path, const std::string &root, const std::string &uri)
{
	std::string res = uri;

	if (*loc_path.begin() == '*')
	{
		res.replace(0, 1, root);
		return (res);
	}
	res.replace(0, 1, root);
	return (res);
}

/*
 * 1. Find location for uri
2. Check if redirect enabled
	return redirect instance;
3. If CGI execute cgi
4. If request method is put return
5. if not found, search for index file;
5. If found return;

 1. Check if file is folder;
 2. If file is folder search for index
 */
const Location *VirtualServer::getLocationFromRequest(HttpRequest &req) const
{
	location_it it;
	location_it s_it;
	std::string path;
	std::string path_with_root;
	std::string index_path;

	path = Utils::normalizeUri(req.getUriNoQuery());
	if (path.empty())
		return (nullptr);

	it = findRouteFromUri(path); //@todo add search for cgi_location
	if (it == _locations.end())
		return (nullptr);
	Utils::removeLocFromUri(it->first, path);

	if (!it->second.getRoot().empty())
		path = getFullPath(it->second.getPath(), it->second.getRoot(), path);
	if (!it->second.getRet().empty())
		return (&it->second);
	if (Utils::isDirectory(path) && !it->second.isAutoindexOn() &&
		!it->second.getIndex().empty())
	{
		if (*path.rbegin() == '/')
			path = path + it->second.getIndex();
		else
			path = path + '/' + it->second.getIndex();
	}
	req.setNormalizedPath(path);
	return (&it->second);
}


VirtualServer::method_type VirtualServer::hashMethod(const std::string &inString)
{
	if (inString == "GET") return VirtualServer::GET;
	if (inString == "POST") return VirtualServer::POST;
	if (inString == "DELETE") return VirtualServer::DELETE;
	return (VirtualServer::OTHER);
}

std::string VirtualServer::unhashMethod(VirtualServer::method_type type)
{
	switch (type)
	{
		case GET :
			return ("GET");
		case POST:
			return ("POST");
		case DELETE:
			return ("DELETE");
		default:
			return ("OTHER");
	}
}

//process request

VirtualServer::VirtualServerException::VirtualServerException(const std::string &msg) : m_msg(msg)
{}

VirtualServer::VirtualServerException::~VirtualServerException() throw()
{}

const char *VirtualServer::VirtualServerException::what() const throw()
{
	std::cerr << "Server config error: ";
	return (m_msg.data());
}



