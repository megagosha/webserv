//
// Created by Elayne Debi on 9/16/21.
//
#include "utils.hpp"

bool fileExistsAndReadable(const std::string &name)
{
	if (FILE *file = fopen(name.c_str(), "r"))
	{
		fclose(file);
		return true;
	} else
	{
		return false;
	}
}

std::string recv(int bytes, int socket)
{
	std::string output(bytes, 0);
	if (read(socket, &output[0], bytes) < 0)
	{
		std::cerr << "Failed to read data from socket.\n";
	}
	return output;
}

bool fileExistsAndExecutable(const char *file)
{
	struct stat st = {};

	if (stat(file, &st) < 0)
		return false;
	if ((st.st_mode & S_IEXEC) != 0)
		return true;
	return false;
}

std::list<std::string> str_tokenizer(const std::string &s, char c)
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

std::string &normalize_path(std::string &s)
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

	tokens = str_tokenizer(s, '/');
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
	for (std::list<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it)
	{
		if (*it == "/")
		{
			if (*(++it) == "/")
			{
				s.clear();
				return (s);
			} else
				--it;
		}
		s.append(*it);
	}
	return (s);
}