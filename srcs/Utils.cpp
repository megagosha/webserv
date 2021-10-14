//
// Created by Elayne Debi on 9/16/21.
//
#include <fstream>
#include <netinet/in.h>
#include "Utils.hpp"

bool Utils::fileExistsAndReadable(const std::string &name)
{
	if (FILE *file = fopen(name.c_str(), "r"))
	{
		fclose(file);
		return true;
	} else
	{
		return false;
	}
};

std::string Utils::recv(int bytes, int socket)
{
	std::string output(bytes, 0);
	if (read(socket, &output[0], bytes) < 0)
	{
		std::cerr << "Failed to read data from socket.\n";
	}
	return output;
}

bool Utils::fileExistsAndExecutable(const char *file)
{
	struct stat st = {};

	if (stat(file, &st) < 0)
		return false;
	if ((st.st_mode & S_IEXEC) != 0)
		return true;
	return false;
}

std::list<std::string> Utils::strTokenizer(const std::string &s, char c)
{
	std::list<std::string> tokens;
	std::string res;

	for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
	{
		if (*it == c)
		{
			if (!res.empty())
				tokens.push_back(res);
			res.clear();
			res.push_back(*it);
			tokens.push_back(res);
			res.clear();
			continue;
		}
		res.push_back(*it);
	}
	if (*s.rend() != '/')
		tokens.push_back(res);
	return (tokens);
}

std::string &Utils::normalizePath(std::string &s)
{
	std::list<std::string> tokens;
	std::list<std::string>::reverse_iterator tmp1;
	std::list<std::string>::reverse_iterator it = tokens.rbegin();
	int i = 0;

	if (s[0] != '/')
	{
		s.clear();
		return (s);
	}

	tokens = strTokenizer(s, '/');
	for (; it != tokens.rend(); ++it)
	{
		tmp1 = it;
		if (*it == "..")
			i += 4;
		if (i > 0)
		{
			--tmp1;
			tokens.erase(--(it.base()));
			it = tmp1;
			--i;
		}
	}
	if (i != 0)
	{
		s.clear();
		return (s);
	}
	s.clear();
	for (std::list<std::string>::iterator rit = tokens.begin(); rit != tokens.end(); ++rit)
	{
		if (*rit == "/")
		{
			if (*(++rit) == "/")
			{
				s.clear();
				return (s);
			} else
				--rit;
		}
		s.append(*rit);
	}
	return (s);
};

Utils::GeneralException::GeneralException(const std::string &msg) : m_msg(msg)
{};

Utils::GeneralException::~GeneralException() throw()
{};

const char *Utils::GeneralException::what() const throw()
{
	std::cerr << "Server config error: ";
	return (m_msg.data());
}


void Utils::skipTokens(std::list<std::string>::iterator &it,
					   std::list<std::string>::iterator &end, int num)
{
	for (int i = 0; i < num; ++i)
	{
		if (it == end)
			throw Utils::GeneralException("Token syntax error");
		++it;
	}
	if (it == end)
		throw Utils::GeneralException("Token syntax error");
}

void Utils::tokenizeFileStream(std::string const &file_path, std::list<std::string> &res)
{
	std::string tok;
	std::ifstream is(file_path, std::ifstream::binary);
	std::string line;

//	char buf[500];
//	std::cout << getcwd(buf, 500);
//	std::cout << file_path << std::endl;
	if ((is.rdstate() & std::ifstream::failbit) != 0)
		throw Utils::GeneralException("Error opening " + file_path + "\n");
	if (is)
		while (std::getline(is, line))
		{
			tok.clear();
			for (std::string::iterator it = line.begin(); it != line.end(); ++it)
			{
				if (std::isspace(*it) || *it == '#' || *it == ';')
				{
					if (!tok.empty())
					{
						res.push_back(std::string(tok));
						tok.clear();
					}
					if (*it == '#')
						break;
					if (*it == ';')
						res.push_back(std::string(1, *it));
					continue;
				}
				tok.push_back(*it);
			}
			if (!tok.empty())
				res.push_back(std::string(tok));
		}
	else
	{
		is.close();
		throw Utils::GeneralException("Error EOF was not reached " + file_path + "\n");
	}
	is.close();
}

std::string Utils::ClientIpFromFd(int fd)
{
	sockaddr_in addr = {};
	socklen_t len;
	char ip[INET6_ADDRSTRLEN];
	const char *str;
	bzero(&addr, sizeof(addr));
	len = sizeof(addr);
	getsockname(fd, (struct sockaddr *) &addr, &len);
	str = inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
	return std::string(ip);
}

std::string Utils::getExt(const std::string &str, char delim)
{
	size_t res;

	res = str.find(delim);
	if (res == std::string::npos)
		return ("");
	else
		return (str.substr(res + 1, str.size()));
}

std::string Utils::getWithoutExt(const std::string &str, char delim)
{
	size_t res;

	res = str.find(delim);
	if (res == std::string::npos)
		return (str);
	else
		return (str.substr(0, res));
}

char **Utils::mapToEnv(const std::map<std::string, std::string> &env)
{
	char **res = new char *[env.size() + 1];

	int i = 0;
	for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it)
	{
		std::string tmp = it->first + "=" + it->second;
		res[i] = new char[tmp.size() + 1];
		strcpy(res[i], tmp.data());
		++i;
	}
	res[i] = nullptr;
	return (res);
}

void Utils::clearNullArr(char **arr)
{
	int i = 0;
	while (arr[i] != nullptr)
	{
		delete[] arr[i];
		i++;
	}
	delete arr;
}