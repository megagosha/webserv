//
// Created by George Tevosov on 10.10.2021.
//

#include "CgiHandler.hpp"

void CgiHandler::setEnv(HttpRequest const &request)
{
	_env["AUTH_TYPE"] = "";
	if (!request.getBody().size()) {
		_env["CONTENT_LENGTH"] = std::to_string(request.getBody().size());
		std::map<std::string, std::string>::const_iterator it = request.getHeaderFields().find("Content-Type");
		if (it != request.getHeaderFields().end())
			_env["CONTENT_TYPE"] = it->second;
		}

	_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	_env["PATH_INFO"] = "";
	_env["PATH_TRANSLATED"] = "";
	_env["QUERY_STRING"] = "";
	_env["REMOTE_ADDR"] = "";
	_env["REMOTE_HOST"] = "";
	_env["REMOTE_IDENT"] = "";
	_env["REMOTE_USER"] = "";
	_env["REQUEST_METHOD"] = "";
	_env["SCRIPT_NAME"] = "";
	_env["SERVER_NAME"] = "";
	_env["SERVER_PORT"] = "";
	_env["SERVER_PROTOCOL"] = "";
	_env["SERVER_SOFTWARE"] = "";
}
