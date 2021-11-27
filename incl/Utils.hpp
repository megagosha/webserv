//
// Created by George Tevosov on 05.10.2021.
//

#ifndef UTILS_HPP
#define UTILS_HPP
# ifndef LOG_LEVEL
# define LOG_LEVEL 2
#endif

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
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Session.hpp"
#include <dirent.h>

struct Utils {

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

    class GeneralException : public std::exception {
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
    static bool folderExistsAndWritable(const std::string &name);
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

enum Code {
    FG_DEFAULT       = 39,
    FG_BLACK         = 30,
    FG_RED           = 31,
    FG_GREEN         = 32,
    FG_YELLOW        = 33,
    FG_BLUE          = 34,
    FG_MAGENTA       = 35,
    FG_CYAN          = 36,
    FG_LIGHT_GRAY    = 37,
    FG_DARK_GRAY     = 90,
    FG_LIGHT_RED     = 91,
    FG_LIGHT_GREEN   = 92,
    FG_LIGHT_YELLOW  = 93,
    FG_LIGHT_BLUE    = 94,
    FG_LIGHT_MAGENTA = 95,
    FG_LIGHT_CYAN    = 96,
    FG_WHITE         = 97,
    BG_RED           = 41,
    BG_GREEN         = 42,
    BG_BLUE          = 44,
    BG_DEFAULT       = 49
};

std::ostream &operator<<(std::ostream &os, Code cod);

std::ostream &operator<<(std::ostream &out, const HttpRequest &c);

std::ostream &operator<<(std::ostream &out, const HttpResponse &c);

std::ostream &operator<<(std::ostream &out, const Session &c);

#endif
