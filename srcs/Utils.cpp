//
// Created by Elayne Debi on 9/16/21.
//
#include <fstream>
#include "Utils.hpp"

bool FtUtils::fileExistsAndReadable(const std::string &name)
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

std::string FtUtils::recv(int bytes, int socket)
{
	std::string output(bytes, 0);
	if (read(socket, &output[0], bytes) < 0)
	{
		std::cerr << "Failed to read data from socket.\n";
	}
	return output;
}

bool FtUtils::fileExistsAndExecutable(const char *file)
{
	struct stat st = {};

	if (stat(file, &st) < 0)
		return false;
	if ((st.st_mode & S_IEXEC) != 0)
		return true;
	return false;
}

std::list<std::string> FtUtils::strTokenizer(const std::string &s, char c)
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

std::string &FtUtils::normalizePath(std::string &s)
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
	std::cout << "check " << s << std::endl;
	return (s);
};

FtUtils::GeneralException::GeneralException(const std::string &msg) : m_msg(msg)
{};

FtUtils::GeneralException::~GeneralException() throw()
{};

const char *FtUtils::GeneralException::what() const throw()
{
	std::cerr << "Server config error: ";
	return (m_msg.data());
}


void FtUtils::skipTokens(std::list<std::string>::iterator &it,
						 std::list<std::string>::iterator &end, int num)
{
	for (int i = 0; i < num; ++i)
	{
		if (it == end)
			throw FtUtils::GeneralException("Token syntax error");
		++it;
	}
	if (it == end)
		throw FtUtils::GeneralException("Token syntax error");
}

void FtUtils::tokenizeFileStream(std::string const &file_path, std::list<std::string> &res)
{
	std::string tok;
	std::ifstream is(file_path, std::ifstream::binary);
	std::string line;

	std::cout << file_path << std::endl;
	if ((is.rdstate() & std::ifstream::failbit) != 0)
		throw FtUtils::GeneralException("Error opening " + file_path + "\n");
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
		throw FtUtils::GeneralException("Error EOF was not reached " + file_path + "\n");
	}
	is.close();
}