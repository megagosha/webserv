//
// Created by Elayne Debi on 10/25/21.
//

#include "FileStats.hpp"

FileStats::FileStats(const std::string &path_to_file) : _file_stats() {
    stat(path_to_file.data(), &_file_stats);
}

std::string FileStats::getTimeModified() {
    struct tm     *clock;
    std::string human_time;

    clock      = gmtime(&(_file_stats.st_mtimespec.tv_sec));
    return (asctime(clock));
}

std::string FileStats::getSizeInMb() const {
    return (std::to_string((float) _file_stats.st_size / 1000000));
}

FileStats::~FileStats() {

}

FileStats::FileStats(const FileStats &rhs) : _file_stats(rhs._file_stats) {

}

FileStats &FileStats::operator=(const FileStats &rhs) {
    if (this == &rhs)
        return (*this);
    _file_stats = rhs._file_stats;
    return (*this);
}
