//
// Created by Elayne Debi on 9/16/21.
//
#include <fstream>
#include <netinet/in.h>
#include <sstream>
#include "Utils.hpp"

bool Utils::fileExistsAndReadable(const std::string &name)
{
	struct stat path_stat;

	stat(name.data(), &path_stat);
	if (FILE *file = fopen(name.c_str(), "r"))
	{
		fclose(file);
		return S_ISREG(path_stat.st_mode);
	} else
	{
		return false;
	}
};

bool Utils::fileExistsAndWritable(const std::string &name)
{
	FILE *file;
	if ((file = std::fopen(name.c_str(), "r+")) != nullptr)
	{
		fclose(file);
		return true;
	} else
	{
		return false;
	}
};

void Utils::recv(long bytes, int socket, std::string &res)
{
	if (read(socket, &res[0], bytes) < 0)
	{ //@todo gracefull close
		std::cout << std::strerror(errno);
		throw GeneralException("Error while reading socket");
	}
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
	std::string            res;

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
	if (*s.rbegin() != '/')
		tokens.push_back(res);
	return (tokens);
}

std::string Utils::normalizeUri(std::string s)
{
	std::list<std::string>                   tokens;
	std::list<std::string>::reverse_iterator tmp1;
	std::list<std::string>::reverse_iterator it = tokens.rbegin();
	int                                      i  = 0;

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
			if (++rit != tokens.end() && *(rit) == "/")
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
	std::string   tok;
	std::ifstream is(file_path, std::ifstream::binary);
	std::string   line;

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

std::string Utils::ClientIpFromSock(sockaddr *addr)
{
//    sockaddr_in addr = {};
//    socklen_t len;
	char ip[INET6_ADDRSTRLEN];
//    const char *str;
//    bzero(&addr, sizeof(addr));
//    len = sizeof(addr);
//    getsockname(fd, (struct sockaddr *) &addr, &len);

	inet_ntop(AF_INET, addr, ip, sizeof(ip));
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

	int                                                     i  = 0;
	for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it)
	{
		if (it->second.empty())
			continue;
		std::string tmp = it->first + "=" + it->second;
		res[i] = new char[tmp.size() + 1];
		strcpy(res[i], tmp.data());
		++i;
	}
	res[i]                                                     = nullptr;
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

bool Utils::isDirectory(const std::string &path)
{
	struct stat s;
	if (stat(path.data(), &s) == 0)
	{
		return (S_ISDIR(s.st_mode));
	}
	return (false);
}

bool Utils::isNotEmptyDirectory(const std::string &path)
{
	int           n    = 0;
	struct dirent *d;
	DIR           *dir = opendir(path.data());
	if (dir == nullptr) //Not a directory or doesn't exist
		return false;
	while ((d = readdir(dir)) != nullptr)
	{
		if (++n > 2)
			break;
	}
	closedir(dir);
	if (n > 2)
		return (true);
	else
		return (false);
}

bool Utils::isFile(const std::string &path)
{
	struct stat s;
	if (stat(path.data(), &s) == 0)
		if (s.st_mode & S_IFREG)
			return (true);
	return (false);
}

bool Utils::checkIfPathExists(const std::string &path)
{
	size_t      pos;
	std::string new_path;

	pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return (false);
	new_path = path.substr(0, pos);
	if (!new_path.empty() && isDirectory(new_path))
		return (true);
	return (false);
}

std::string Utils::getFileNameFromRequest(const std::string &path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return ("");
	else
		return (path.substr(pos + 1));
}

bool
Utils::parse(const std::string &src, std::size_t &token_start, const std::string &token_delim, bool delim_exact,
			 std::size_t max_len,
			 std::string &token)
{
	token_start = src.find_first_not_of(" \t\r\n", token_start);
	if (token_start == std::string::npos)
		return (false);
	std::size_t line_end = src.find_first_of("\r\n", token_start);
	if (line_end == std::string::npos)
		line_end = src.length();
	std::size_t token_end = src.find_first_of(token_delim, token_start);
	if (token_end == std::string::npos && delim_exact)
		return false;
	if (token_end == std::string::npos)
		token_end = line_end;
	token         = src.substr(token_start, token_end - token_start);
	if (token.empty() || token.length() > max_len)
		return (false);
	token_start = token_end;
	return (true);
}

size_t Utils::getContentLength(std::map<std::string, std::string> &headers)
{
	std::map<std::string, std::string>::iterator it;
	size_t                                       res;

	it = headers.find("Content-Length");
	if (it == headers.end())
		return (0);
	res = Utils::toSizeT(it->second.data());
	if (res == std::numeric_limits<size_t>::max())
		return (0);
	return (res);
}

size_t Utils::toSizeT(const char *number)
{
	size_t             sizeT;
	std::istringstream iss(number);
	iss >> sizeT;
	if (iss.fail())
	{
		return std::numeric_limits<size_t>::max();
	} else
	{
		return sizeT;
	}
}

void Utils::removeLocFromUri(const std::string &location, std::string &uri)
{
	typedef std::list<std::string>::iterator l_iter;

	std::list<std::string> list_a = Utils::strTokenizer(uri, '/');
	std::list<std::string> loc_tok = Utils::strTokenizer(location, '/');

	int items_to_delete = 0;
	l_iter it = list_a.begin();
	for (l_iter loc_it = loc_tok.begin(); loc_it != loc_tok.end(); ++loc_it)
	{
		//		std::cout << "yo" << std::endl;
		if  (it != list_a.end())
		{
			//			std::cout << "loc " << *loc_it << " tok " << *it << " comp " << (*it == *loc_it )<< std::endl;
			if ( *it != "/" && *it == *loc_it)
				++items_to_delete;
			++it;
		}
		else
			break;
	}
	//	std::cout << items_to_delete;
	l_iter tmp;
	for (it = list_a.begin(); it != list_a.end() && items_to_delete > 0; )
	{
		tmp = it;
		++tmp;
		if (*it != "/")
			--items_to_delete;
		list_a.erase(it);
		it = tmp;
	}
	uri.clear();
	for (it = list_a.begin(); it != list_a.end(); ++it)
	{
		uri += *it;

	}
	if (uri.empty())
		uri = "/";
	return;
}
//int Utils::countFilesInFolder(const std::string &path) {
//    DIR *dp;
//    int i = 0;
//
//    dp = opendir(path.data());
//
//    if (dp != NULL) {
//        while (readdir(dp) != nullptr) {
//            i++;
//        }
//        closedir(dp);
//    }
//    return (i);
//}