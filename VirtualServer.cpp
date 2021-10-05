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


bool VirtualServer::Location::isAutoindexOn() const
{
	return _autoindex_on;
}

VirtualServer::method_type VirtualServer::hash_method(std::string const &inString)
{
	if (inString == "GET") return GET;
	if (inString == "POST") return POST;
	if (inString == "DELETE") return DELETE;
	return (OTHER);
}

//	bool VirtualServer::Location::isAutoindexOn() const
//	{
//		return _autoindex_on;
//	}

bool VirtualServer::Location::isFileUploadOn() const
{
	return !_file_upload.empty();
}

const std::string &VirtualServer::Location::getIndex() const
{
	return _index;
}

const std::string &VirtualServer::Location::getRoot() const
{
	return _root;
}

const std::string &VirtualServer::Location::getCgiPass() const
{
	return _cgi_pass;
}

const std::vector<bool> &VirtualServer::Location::getMethods() const
{
	return _methods;
}

const std::string &VirtualServer::Location::getRet() const
{
	return _ret;
}

VirtualServer::Location::Location() : _autoindex_on(true),
									  _file_upload(""),
									  _index("index.html"), _root("/"), _cgi_pass(""), _ret("")
{
	_methods.insert(_methods.begin(), 3, true);
}

VirtualServer::Location::Location(const Location &rhs) : _autoindex_on(rhs._autoindex_on),
														 _file_upload(rhs._file_upload),
														 _index(rhs._index), _root(rhs._root), _cgi_pass(""),
														 _ret(rhs._ret)
{
	_methods = rhs._methods;
}

VirtualServer::Location &VirtualServer::Location::operator=(const Location &rhs)
{
	if (this != &rhs)
	{
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

VirtualServer::Location::~Location()
{
}

bool VirtualServer::Location::methodAllowed(const std::string &method)
{
	switch (VirtualServer::hash_method(method))
	{
		case GET:
			return (_methods[0]);
		case POST:
			return (_methods[1]);
		case DELETE:
			return (_methods[2]);
		default:
			return (false);
	}
}

void VirtualServer::Location::setLocation(std::list<std::string>::iterator &it,
										  std::list<std::string>::iterator &end, std::string path)
{

	skip_tok(it, end, 2);

	if (path[0] == '*' && *it == "cgi_pass")
	{
		_cgi_pass = *it;
		skip_tok(it, end, 2);
		//                location.insert(std::make_pair(path, loc));
		skip_tok(it, end, 1);
		return;
	}
	std::list<std::string>::iterator check;
	while (it != end && *it != "}")
	{
		check = it;
		if (*it == "root" && (++it) != end)
		{
			_root = *it;
			skip_tok(it, end, 2);
		}
		if (*it == "methods")
		{
			skip_tok(it, end, 1);

			_methods[0] = false;
			_methods[1] = false;
			_methods[2] = false;
			while (*it != ";")
			{
				switch (VirtualServer::hash_method(*it))
				{
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
		if (*it == "autoindex")
		{
			skip_tok(it, end, 1);
			if (*it == "on" || *it == "off")
				_autoindex_on = *it == "on";
			else
				throw VirtualServerException("Autoindex error: ;");
			skip_tok(it, end, 2);
		}
		if (*it == "return")
		{
			skip_tok(it, end, 1);
			_ret = *it;
			skip_tok(it, end, 2);
		}
		if (*it == "index")
		{
			skip_tok(it, end, 1);
			_index = *it;
			skip_tok(it, end, 2);
		}
		if (*it == "file_upload")
		{
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


uint16_t VirtualServer::getPort() const
{
	return _port;
}

bool VirtualServer::validatePort() const
{
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
		if (!fileExistsAndReadable((*it).second))
			return (false);
	}
	return (true);
}

bool VirtualServer::validateLocations() const
{
	std::string ret;
	for (std::map<std::string, Location>::const_iterator it = _locations.begin(); it != location.end(); ++it)
	{
		if (it->first[0] == '*')
		{
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

const std::map<std::string, VirtualServer::Location> &VirtualServer::getLocations() const
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

void VirtualServer::skip_tok(std::list<std::string>::iterator &it,
							 std::list<std::string>::iterator &end, int num)
{
	for (int i = 0; i < num; ++i)
	{
		if (it == end)
			throw VirtualServerException("Token syntax error");
		++it;
	}
	if (it == end)
		throw VirtualServerException("Token syntax error");
}

void VirtualServer::setHost(std::list<std::string>::iterator &it,
							std::list<std::string>::iterator &end)
{
	skip_tok(it, end, 1);
	_host = inet_addr((*it).data());
	skip_tok(it, end, 2);
}

void VirtualServer::setPort(std::list<std::string>::iterator &it,
							std::list<std::string>::iterator &end)
{
	unsigned int port = 0;

	skip_tok(it, end, 1);
	port = std::stoi((*it));
	_port = (uint16_t) port;
	skip_tok(it, end, 2);
}

void VirtualServer::setBodySize(std::list<std::string>::iterator &it,
								std::list<std::string>::iterator &end)
{
	unsigned long body_size = 0;

	skip_tok(it, end, 1);
	body_size = std::stoi(*it);
	_body_size_limit = body_size;
	skip_tok(it, end, 2);
}

void VirtualServer::setErrorPage(std::list<std::string>::iterator &it,
								 std::list<std::string>::iterator &end)
{
	short err;
	std::string path;

	skip_tok(it, end, 1);
	err = (short) std::stoi(*it);
	skip_tok(it, end, 1);
	_error_pages.insert(std::make_pair(err, *it));
	skip_tok(it, end, 2);
}

void VirtualServer::setLocation(std::list<std::string>::iterator &it,
								std::list<std::string>::iterator &end)
{
	Location loc;
	std::string path;

	skip_tok(it, end, 1);
	path = *it;
	loc.setLocation(it, end, path);
	location.insert(std::make_pair(path, loc));
}


std::map<std::string, VirtualServer::Location>::iterator VirtualServer::findRouteFromUri(std::string &normalized_uri)
{
	if (normalized_uri.empty())
		return (_locations.end());

	int i = 0;
	int best = 0;
	int cur_best = 0;
	std::map<std::string, VirtualServer::Location>::iterator it = _locations.begin();
	std::map<std::string, VirtualServer::Location>::iterator search_res;

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
				cur_best = best;
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
HttpResponse VirtualServer::generate(const HttpRequest &request, std::string host)
{
	std::string req_no_query;
	std::map<std::string, Location>::iterator it;

	req_no_query = request.getRequestUri().substr(0, req_no_query.find('?'));
	if (req_no_query.empty())
		return (HttpResponse(400, *this));
	req_no_query = normalize_path(req_no_query);
	std::map<std::string, VirtualServer::Location>::iterator loc_route;
	loc_route = findRouteFromUri(req_no_query);
	if (loc_route == _locations.end())
		return (HttpResponse(400, *this));

	if (!loc_route->second.methodAllowed(request.getMethod()))
	{
		//https://datatracker.ietf.org/doc/html/rfc7231#section-6.5.5
		//@todo construct Allow _header with supported methods;
		return (HttpResponse(405, *this));
	}
	//@todo validate root field in each location
	req_no_query = req_no_query.replace(0, 1, loc_route->second.getRoot());

	if (*request.getRequestUri().rend() == '/')
	{
		std::string index = loc_route->second.getIndex();
		if (!index.empty())
			req_no_query = req_no_query + index;
		else if (loc_route->second.isAutoindexOn() && request.getMethod() == "GET")
			//create directory listing page
			return (HttpResponse(req_no_query));
		//index enabled
		return (HttpResponse(400, *this));
	} else
	{
		return (HttpResponse(req_noq_query, *this, loc_route->second));
	}
	//		response._proto = "HTTP/1.1";
	//		std::ifstream file(req_no_query, std::ios::binary | std::ios::ate);
	//		std::streamsize size = file.tellg();
	//		file.seekg(0, std::ios::beg);
	//		response.result.reserve(size);
	//		if (file.read(response.result.data(), size))
	//		{
	//			response.insertHeader("Content-Length", std::to_string(size));
	//			response.insertHeader("Content-Type", "text/html"); //@todo find correct mime type
	//			return (response);
	//		} else
	//			return (HttpResponse(404, *this));
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



