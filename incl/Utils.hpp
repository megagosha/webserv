//
// Created by George Tevosov on 05.10.2021.
//

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <cstdio>
#include <sys/stat.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <list>

struct FtUtils
{

public:
	static bool fileExistsAndReadable(const std::string &name);

	static std::string recv(int bytes, int socket);

	static bool fileExistsAndExecutable(const char *file);

	static std::list<std::string> strTokenizer(const std::string &s, char c);

	static std::string &normalizePath(std::string &s);

	static void skipTokens(std::list<std::string>::iterator &it,
						   std::list<std::string>::iterator &end, int num);

	static void tokenizeFileStream(std::string const &file_path,
											std::list<std::string> &res);

	class GeneralException : public std::exception
	{
		const std::string m_msg;
	public:
		GeneralException(const std::string &msg);

		~GeneralException() throw();

		const char *what() const throw();
	};
};

#endif
