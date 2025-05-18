#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <string>
#include <unordered_map>

extern std::unordered_map<std::string, std::string> user_db;

bool authenticateUser(const std::string& credentials, std::string& username);

#endif // AUTHENTICATION_H
