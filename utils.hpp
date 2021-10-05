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


bool fileExistsAndReadable(const std::string &name);

std::string recv(int bytes, int socket);

bool fileExistsAndExecutable(const char *file);

std::list<std::string> str_tokenizer(const std::string &s, char c);

std::string &normalize_path(std::string &s);

#endif //WEBSERV_UTILS_HPP
