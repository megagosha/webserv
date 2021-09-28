//
// Created by Elayne Debi on 9/16/21.
//
#include <string>
#include <cstdio>
#include <sys/stat.h>

bool fileExistsAndReadable (const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
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