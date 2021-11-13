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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <map>
#include <sys/types.h>
#include <csignal>

#include <dirent.h>

struct Utils
{

public:
	static bool fileExistsAndReadable(const std::string &name);

	static void recv(long bytes, int socket, std::string &res);

	static bool fileExistsAndExecutable(const char *file);

	static std::list<std::string> strTokenizer(const std::string &s, char c);

	static std::string normalizeUri(std::string s);

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

	static std::string ClientIpFromSock(sockaddr *addr);

	static std::string getExt(const std::string &str, char delim);

	static std::string getWithoutExt(const std::string &str, char delim);

	static std::string getFileNameFromRequest(const std::string &path);

	static char **mapToEnv(const std::map<std::string, std::string> &env);

	static void clearNullArr(char **arr);

	static bool isFile(const std::string &path);

	static bool isDirectory(const std::string &path);

	static bool fileExistsAndWritable(const std::string &name);

	static bool isNotEmptyDirectory(const std::string &path);

	static bool checkIfPathExists(const std::string &path);

	static bool
	parse(const std::string &src, std::size_t &token_start, const std::string &token_delim, bool delim_exact,
		  std::size_t max_len,
		  std::string &token);
	static size_t getContentLength(std::map<std::string, std::string> &headers);
	static size_t toSizeT(const char *number);

	static void removeLocFromUri(const std::string &location, std::string &uri);

};

#endif
