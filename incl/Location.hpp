//
// Created by George Tevosov on 06.10.2021.
//

#ifndef WEBSERV_LOCATION_HPP
#define WEBSERV_LOCATION_HPP
#include <string>
#include <vector>
#include <list>
#include "VirtualServer.hpp"
#include "Utils.hpp"
class Location
{
private:
	bool _autoindex_on;
	std::string _file_upload;
	std::string _index;
	std::string _root;
	std::string _cgi_pass;
	std::vector<bool> _methods;//0 -> get 1 -> post 2 -> delete
	std::string _ret;
public:
	bool isAutoindexOn() const;

	bool isFileUploadOn() const;

	const std::string &getIndex() const;

	const std::string &getRoot() const;

	const std::string &getCgiPass() const;

	const std::vector<bool> &getMethods() const;

	const std::string &getRet() const;

	Location();

	Location(const Location &rhs);

	Location &operator=(const Location &rhs);

	~Location();

	bool methodAllowed(const std::string &method);

	void setLocation(std::list<std::string>::iterator &it,
					 std::list<std::string>::iterator &end, std::string path);

    std::string     getAllowedMethodsField(void) const;
};

#endif //WEBSERV_LOCATION_HPP
