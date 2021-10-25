//
// Created by Elayne Debi on 10/25/21.
//

#ifndef WEBSERV_FILESTATS_HPP
#define WEBSERV_FILESTATS_HPP
#include <string>
#include <iostream>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctime>

class FileStats {
    struct stat _file_stats;
public:
    FileStats(const std::string &path_toFile);
    std::string getTimeModified();
    std::string getSizeInMb() const;
    ~FileStats();
    FileStats(const FileStats &rhs);
    FileStats &operator=(const FileStats &rhs);
};


#endif //WEBSERV_FILESTATS_HPP
