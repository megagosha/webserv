cmake_minimum_required(VERSION 3.19)
project(webserv)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

include_directories(./incl)

add_compile_options(-Wall -Wextra -Werror)

add_executable(webserv
        srcs/Session.cpp
        srcs/HttpRequest.cpp
        srcs/HttpResponse.cpp
        srcs/KqueueEvents.cpp
        srcs/Server.cpp
        srcs/Socket.cpp
        srcs/VirtualServer.cpp
        srcs/Location.cpp
        srcs/Utils.cpp
        srcs/MimeType.cpp
#        srcs/CgiHandler.cpp
        srcs/check.cpp

        incl/HttpRequest.hpp
        incl/HttpResponse.hpp
        incl/KqueueEvents.hpp
        incl/Server.hpp
        incl/Socket.hpp
        incl/Utils.hpp
        incl/VirtualServer.hpp
        incl/Location.hpp
        incl/MimeType.hpp
        incl/Session.hpp
#        incl/CgiHandler.hpp
        )
