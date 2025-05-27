#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <string>
#include <unordered_map>

extern std::unordered_map<std::string, std::string> user_db;

bool authenticateUser(const std::string& credentials, std::string& username);
bool registerUser(const std::string &username, const std::string &password);
void initDatabase();

#endif // AUTHENTICATION_H
