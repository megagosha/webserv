//
// Created by George Tevosov on 09.10.2021.
//

#include "MimeType.hpp"

//MimeType::~MimeType()
//{
//
//}
std::multimap<std::string, std::string> MimeType::_types;

MimeType::MimeType(const std::string &path_to_conf)
{
	std::list<std::string> res;

	Utils::tokenizeFileStream(path_to_conf, res);
	std::list<std::string>::iterator it = res.begin();
	std::list<std::string>::iterator end = res.end();
	if (*it != "types")
		throw MimeTypeException("Mime config error");
	else
		Utils::skipTokens(it, end, 2);
	for (; it != end; ++it)
	{
		if (*it == "}")
			break;
		std::string mime_type;
		mime_type = *it;
		Utils::skipTokens(it, end, 1);
		while (*it->rbegin() != ';')
		{
			_types.insert(std::make_pair(*it, mime_type));
			Utils::skipTokens(it, end, 1);
		}
		if (*(it->rbegin()) == ';')
		{
			it->pop_back();
			_types.insert(std::make_pair(*it, mime_type));
		}
		else
			throw MimeTypeException("Mime confing syntax error");
	}
}

//MimeType &MimeType::operator=(const MimeType &rhs)
//{
//	if (this == &rhs)
//		return (*this);
//	return *this;
//}

const char *MimeType::getType(const std::string &file_path)
{
	std::cout << file_path << std::endl;
	std::string ext = file_path.substr(file_path.find_last_of('.') + 1);
	if (ext.empty())
		return ("application/octet-stream");
	std::multimap<std::string, std::string>::iterator res = _types.find(ext);
	if (res == _types.end())
		return ("application/octet-stream");
	std::cout << "Found :" << res->second << std::endl;
	return res->second.data();
}


MimeType::MimeTypeException::MimeTypeException(const std::string &msg) : m_msg(msg)
{

}

MimeType::MimeTypeException::~MimeTypeException() throw()
{

}

const char *MimeType::MimeTypeException::what() const throw()
{
	std::cerr << "ServerError: ";
	return exception::what();
}
