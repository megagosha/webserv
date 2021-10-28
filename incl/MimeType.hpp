//
// Created by George Tevosov on 09.10.2021.
//

#ifndef MIMETYPE_HPP
#define MIMETYPE_HPP
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include "Utils.hpp"

class MimeType {
protected:
	static std::multimap<std::string, std::string> _types;
public:
	MimeType(std::string const &path_to_conf);

	static const char *getType(std::string const &file_path);

	static std::string getFileExtension(const std::string &type);

	class MimeTypeException : public std::exception
			{
		const std::string m_msg;
			public:
				MimeTypeException(const std::string &msg);

				~MimeTypeException() throw();

				const char *what() const throw();
			};
};

#endif
