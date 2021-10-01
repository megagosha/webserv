//
// Created by Elayne Debi on 9/16/21.
//
#include <string>
#include <cstdio>
#include <sys/stat.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <list>
bool fileExistsAndReadable (const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
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
    struct stat  st = {};

    if (stat(file, &st) < 0)
        return false;
    if ((st.st_mode & S_IEXEC) != 0)
        return true;
    return false;
}

std::list<std::string> str_tokenizer(const std::string &s, char c) {
    std::list<std::string> tokens;
    std::string res;

    for (std::string::const_iterator it = s.begin(); it != s.end(); ++it) {
        if (*it == c) {
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

std::pair<std::string, bool> normalize_path(const std::string &s) {
    std::list<std::string> tokens;
    std::list<std::string>::reverse_iterator tmp1;
    std::list<std::string>::reverse_iterator it = tokens.rbegin();
    std::pair<std::string, bool> res;
    int i = 0;

    res.second = false;
    if (s[0] != '/')
        return (res);

    tokens = str_tokenizer(s, '/');
    for (; it != tokens.rend(); ++it) {
        tmp1 = it;
        if (*it == "..")
            i += 4;
        if (i > 0) {
            --tmp1;
            tokens.erase(--(it.base()));
            it = tmp1;
            --i;
        }
    }
    if (i != 0)
        return (res);
    for (std::list<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it) {
        if (*it == "/") {
            if (*(++it) == "/")
                return (res);
            else
                --it;
        }
        res.first.append(*it);
    }
    res.second = true;
    std::cout << res.first << std::endl;
    return (res);
}