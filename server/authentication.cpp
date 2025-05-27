#include "authentication.h"
#include <algorithm>
#include <sqlite3.h>
#include <iostream>

// std::unordered_map<std::string, std::string> user_db = {
//     {"alice", "pass123"},
//     {"bob", "qwerty"},
//     {"charlie", "chatme"}};

const char *DB_PATH = "../database/appDatabase.db";

std::string trim_newlines(const std::string &str)
{
    size_t end = str.find_last_not_of("\n\r");
    if (end == std::string::npos)
        return "";
    return str.substr(0, end + 1);
}

void initDatabase()
{
    sqlite3 *db;
    char *errMsg = nullptr;

    if (sqlite3_open(DB_PATH, &db) == SQLITE_OK)
    {
        const char *sql = "CREATE TABLE IF NOT EXISTS users ("
                          "username TEXT PRIMARY KEY,"
                          "password TEXT NOT NULL"
                          ");";

        if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
        {
            std::cerr << "Error creating table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
        else
        {
            std::cout << "Database initialized successfully.\n";
        }

        sqlite3_close(db);
    }
    else
    {
        std::cerr << "Can't open database.\n";
    }
}

bool authenticateUser(const std::string &credentials, std::string &username)
{
    std::string trimmed = trim_newlines(credentials);
    size_t delimiterPos = trimmed.find(':');

    if (delimiterPos == std::string::npos)
        return false;

    std::string user = trimmed.substr(0, delimiterPos);
    std::string pass = trimmed.substr(delimiterPos + 1);

    // if (user_db.count(user) && user_db[user] == pass)
    // {
    //     username = user;
    //     return true;
    // }

    sqlite3 *db;
    sqlite3_stmt *stmt;
    bool authenticated = false;

    if (sqlite3_open(DB_PATH, &db) == SQLITE_OK)
    {
        std::string sql = "SELECT password FROM users WHERE username = ?;";
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                const char *stored_pass = (const char *)sqlite3_column_text(stmt, 0);
                if (stored_pass && pass == stored_pass)
                {
                    username = user;
                    authenticated = true;
                }
            }
        }
        else
        {
            std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
    else
    {
        std::cerr << "Can't open database.\n";
    }

    return authenticated;
}

bool registerUser(const std::string &username, const std::string &password)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    bool success = false;

    if (sqlite3_open(DB_PATH, &db) == SQLITE_OK)
    {
        // Check if user already exists
        std::string check_sql = "SELECT COUNT(*) FROM users WHERE username = ?;";
        if (sqlite3_prepare_v2(db, check_sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                int count = sqlite3_column_int(stmt, 0);
                sqlite3_finalize(stmt);

                if (count > 0)
                {
                    sqlite3_close(db);
                    return false; // User already exists
                }
            }
        }
        else
        {
            std::cerr << "SQL prepare error (check): " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return false;
        }

        // Insert new user
        std::string insert_sql = "INSERT INTO users (username, password) VALUES (?, ?);";
        if (sqlite3_prepare_v2(db, insert_sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_DONE)
            {
                success = true;
            }
            else
            {
                std::cerr << "SQL insert error: " << sqlite3_errmsg(db) << std::endl;
            }

            sqlite3_finalize(stmt);
        }
        else
        {
            std::cerr << "SQL prepare error (insert): " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_close(db);
    }
    else
    {
        std::cerr << "Can't open database.\n";
    }

    return success;
}