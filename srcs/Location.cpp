//
// Created by George Tevosov on 06.10.2021.
//

#include "Location.hpp"

bool Location::isAutoindexOn() const
{
	return _autoindex_on;
}

VirtualServer::method_type hashMethod(std::string const &inString)
{
	if (inString == "GET") return VirtualServer::GET;
	if (inString == "POST") return VirtualServer::POST;
	if (inString == "DELETE") return VirtualServer::DELETE;
	return (VirtualServer::OTHER);
}

std::string Location::getAllowedMethodsField() const
{
    std::string res;

    if (_methods[2])
        res.insert(0, "DELETE");
    if (_methods[1])
    {
        if (!res.empty())
            res.insert(0, ", ");
        res.insert(0, "POST");
    }
    if (_methods[0])
    {
        if (!res.empty())
            res.insert(0, ", ");
        res.insert(0, "GET");
    }
    return (res);
}

//	bool VirtualServer::Location::isAutoindexOn() const
//	{
//		return _autoindex_on;
//	}

bool Location::isFileUploadOn() const
{
	return !_file_upload.empty();
}

const std::string &Location::getFileUploadPath() const
{
    return (_file_upload);
}

const std::string &Location::getIndex() const
{
	return _index;
}

const std::string &Location::getRoot() const
{
	return _root;
}

const std::string &Location::getCgiPass() const
{
	return _cgi_pass;
}

const std::vector<bool> &Location::getMethods() const
{
	return _methods;
}

const std::string &Location::getRet() const
{
	return _ret;
}

Location::Location() : _autoindex_on(true),
					   _file_upload(""),
					   _index("index.html"), _root("/"), _cgi_pass(""), _ret("")
{
	_methods.reserve(3);
    _methods[0] = true;
	_methods.insert(_methods.begin() + 1, 2, false);
}

Location::Location(const Location &rhs) : _autoindex_on(rhs._autoindex_on),
										  _file_upload(rhs._file_upload),
										  _index(rhs._index), _root(rhs._root), _cgi_pass(rhs._cgi_pass),
										  _ret(rhs._ret)
{
	_methods = rhs._methods;
}

Location &Location::operator=(const Location &rhs)
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

Location::~Location()
{
}

bool Location::methodAllowed(const std::string &method) const
{
	switch (hashMethod(method))
	{
		case VirtualServer::GET:
			return (_methods[0]);
		case VirtualServer::POST:
			return (_methods[1]);
		case VirtualServer::DELETE:
			return (_methods[2]);
		default:
			return (false);
	}
}

void Location::setLocation(std::list<std::string>::iterator &it,
						   std::list<std::string>::iterator &end, std::string path)
{

	Utils::skipTokens(it, end, 2);

//	if (path[0] == '*' && *it == "cgi_pass")
//	{
//		Utils::skipTokens(it, end, 1);
//        _cgi_pass = *it;
//        //                location.insert(std::make_pair(path, loc));
//		Utils::skipTokens(it, end, 2);
//		return;
//	}
	std::list<std::string>::iterator check;
	while (it != end && *it != "}")
	{
		check = it;
		if (*it == "root" && (++it) != end)
		{
			_root = *it;
			Utils::skipTokens(it, end, 2);
		}
	  	if (path[0] == '*' && *it == "cgi_pass")
		{
			Utils::skipTokens(it, end, 1);
			_cgi_pass = *it;
			//                location.insert(std::make_pair(path, loc));
			std::cout << "cgi parsed" << *it << std::endl;
			Utils::skipTokens(it, end, 2);
		}
		if (*it == "methods")
		{
			Utils::skipTokens(it, end, 1);

			_methods[0] = true;
			_methods[1] = false;
			_methods[2] = false;
			while (*it != ";")
			{
				switch (hashMethod(*it))
				{
					case VirtualServer::GET:
						_methods[0] = true;
						break;
					case VirtualServer::POST:
						_methods[1] = true;
						break;
					case VirtualServer::DELETE:
						_methods[2] = true;
						break;
					default:
						throw VirtualServer::VirtualServerException("Method not supported " + *it);
				}
				Utils::skipTokens(it, end, 1);
			}
			Utils::skipTokens(it, end, 1);
		}
		if (*it == "autoindex")
		{
			Utils::skipTokens(it, end, 1);
			if (*it == "on" || *it == "off")
				_autoindex_on = *it == "on";
			else
				throw VirtualServer::VirtualServerException("Autoindex error: ;");
			Utils::skipTokens(it, end, 2);
		}
		if (*it == "return")
		{
			Utils::skipTokens(it, end, 1);
			_ret = *it;
			Utils::skipTokens(it, end, 2);
		}
		if (*it == "index")
		{
			Utils::skipTokens(it, end, 1);
			_index = *it;
			Utils::skipTokens(it, end, 2);
		}
		if (*it == "file_upload")
		{
			Utils::skipTokens(it, end, 1);
			_file_upload = *it;
			Utils::skipTokens(it, end, 2);
		}
		if (it != end && *it == "}")
			break;
		if (it == check)
			throw VirtualServer::VirtualServerException("Error location parsing");

	}
	if (it == end || *it != "}")
		throw VirtualServer::VirtualServerException("Syntax error: }");
	Utils::skipTokens(it, end, 1);
}
