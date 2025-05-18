#include "authentication.h"
#include <algorithm>

std::unordered_map<std::string, std::string> user_db = {
    {"alice", "pass123"},
    {"bob", "qwerty"},
    {"charlie", "chatme"}};

std::string trim_newlines(const std::string &str)
{
    return str.substr(0, str.find_last_not_of("\n\r") + 1);
}

bool authenticateUser(const std::string &credentials, std::string &username)
{
    std::string trimmed = trim_newlines(credentials);
    size_t delimiterPos = trimmed.find(':');

    if (delimiterPos == std::string::npos)
        return false;

    std::string user = trimmed.substr(0, delimiterPos);
    std::string pass = trimmed.substr(delimiterPos + 1);

    if (user_db.count(user) && user_db[user] == pass)
    {
        username = user;
        return true;
    }

    return false;
}