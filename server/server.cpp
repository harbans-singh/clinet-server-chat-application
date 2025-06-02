// ===== server.cpp =====
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include "authentication.h"

#define SERVER_PORT 8080

std::unordered_map<int, std::string> clients;
std::mutex clients_mutex;
std::mutex auth_clients_mutex;
std::unordered_map<int, std::string> authenticated_clients;

void broadcast_message(const std::string &message, int sender_fd)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto& [client_fd, client_name] : clients)
    {
        if (client_fd != sender_fd)
        {
            send(client_fd, message.c_str(), message.length(), 0);
        }
    }
}

void broadcast_user_list(int sender_fd)
{
    std::string user_list = "USER_LIST_UPDATE:";
    bool first = true;
    for (const auto &entry : clients)
    {
        if (!first)
            user_list += ",";
        user_list += entry.second;
        first = false;
    }
    broadcast_message(user_list, sender_fd);
}

void handle_client(int client_socket)
{

    char buffer[1024];
    bool authenticated = false;
    std::string username;

    // Authentication loop
    while (!authenticated)
    {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_socket, buffer, sizeof(buffer));
        if (valread <= 0)
        {
            close(client_socket);
            return;
        }

        std::string received(buffer);
        received.erase(received.find_last_not_of("\n\r") + 1);

        if (received == "REGISTER")
        {
            std::string ask = "SEND_NEW_CREDENTIALS\n";
            send(client_socket, ask.c_str(), ask.length(), 0);

            memset(buffer, 0, sizeof(buffer));
            valread = read(client_socket, buffer, sizeof(buffer));
            if (valread <= 0)
            {
                close(client_socket);
                return;
            }

            std::string newCred(buffer);
            newCred.erase(newCred.find_last_not_of("\n\r") + 1);

            size_t delimiterPos = newCred.find(':');
            if (delimiterPos == std::string::npos)
            {
                std::string fail = "REGISTER_FAILED: Invalid format\n";
                send(client_socket, fail.c_str(), fail.length(), 0);
                continue;
            }

            std::string newUser = newCred.substr(0, delimiterPos);
            std::string newPass = newCred.substr(delimiterPos + 1);

            if (registerUser(newUser, newPass))
            {
                authenticated = true;
                username = newUser;

                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    if (clients.find(client_socket) == clients.end())
                    {
                        clients.insert({client_socket, username});
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(auth_clients_mutex);
                    authenticated_clients[client_socket] = username;
                }

                std::string success = "REGISTER_SUCCESS\n";
                send(client_socket, success.c_str(), success.length(), 0);
                broadcast_user_list(client_socket);
            }
            else
            {
                std::string fail = "REGISTER_FAILED: User exists\n";
                send(client_socket, fail.c_str(), fail.length(), 0);
            }
        }
        else
        {
            if (authenticateUser(received, username))
            {
                authenticated = true;
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    if (clients.find(client_socket) == clients.end())
                    {
                        clients.insert({client_socket, username});
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(auth_clients_mutex);
                    authenticated_clients[client_socket] = username;
                }

                std::string success = "LOGIN_SUCCESS\n";
                send(client_socket, success.c_str(), success.length(), 0);
                broadcast_user_list(client_socket);
            }
            else
            {
                std::string fail = "LOGIN_FAILED\n";
                send(client_socket, fail.c_str(), fail.length(), 0);
            }
        }
    }

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = read(client_socket, buffer, sizeof(buffer));
        if (bytes_received <= 0)
        {
            break;
        }

        std::string raw_message(buffer);
        raw_message.erase(raw_message.find_last_not_of("\n\r") + 1);

        if (raw_message.rfind("/", 0) == 0)
        {
            std::istringstream iss(raw_message);
            std::string command;
            iss >> command;

            if (command == "/whisper")
            {
                std::string target_user;
                iss >> target_user;
                std::string private_msg;
                getline(iss, private_msg);

                bool found = false;
                for (auto &[fd, uname] : authenticated_clients)
                {
                    if (uname == target_user)
                    {
                        std::string message = "(Private) " + username + ":" + private_msg + "\n";
                        send(fd, message.c_str(), message.size(), 0);
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    std::string err = "User not found: " + target_user + "\n";
                    send(client_socket, err.c_str(), err.size(), 0);
                }
            }
            else if (command == "/list")
            {
                std::string user_list = "Online Users:\n";
                std::lock_guard<std::mutex> lock(auth_clients_mutex);
                for (auto &[fd, uname] : authenticated_clients)
                {
                    user_list += "- " + uname + "\n";
                }
                send(client_socket, user_list.c_str(), user_list.size(), 0);
            }
            else if (command == "/exit")
            {
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    clients.erase(client_socket);
                }

                {
                    std::lock_guard<std::mutex> lock(auth_clients_mutex);
                    authenticated_clients.erase(client_socket);
                }
                std::string msg = username + " has left the chat.\n";
                broadcast_message(msg, client_socket);
                broadcast_user_list(client_socket);
                close(client_socket);
                return;
            }
            else if (command == "/help")
            {
                std::string help =
                    "Available commands:\n"
                    "/whisper <user> <msg> - Private message\n"
                    "/list - Show online users\n"
                    "/exit - Exit chat\n"
                    "/help - Show this help\n";
                send(client_socket, help.c_str(), help.size(), 0);
            }
            else
            {
                std::string err = "Unknown command. Use /help to see available commands.\n";
                send(client_socket, err.c_str(), err.size(), 0);
            }

            continue;
        }
        else
        {
            std::string message = username + ": " + raw_message + "\n";
            broadcast_message(message, client_socket);
        }
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(client_socket);
        {
            std::lock_guard<std::mutex> lock(auth_clients_mutex);
            authenticated_clients.erase(client_socket);
        }
        std::string fail = "LOGIN_FAILED\n";
        send(client_socket, fail.c_str(), fail.length(), 0);
    }
    broadcast_user_list(client_socket);
    close(client_socket);
}

int main()
{
    initDatabase();
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);
    std::cout << "Server is listening on port: " << SERVER_PORT << std::endl;

    while (true)
    {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0)
        {
            perror("accept");
            continue;
        }

        std::cout << "New client connected: " << new_socket << "\n";
        std::thread client_thread(handle_client, new_socket);
        client_thread.detach();
    }

    close(server_fd);
    return 0;
}
