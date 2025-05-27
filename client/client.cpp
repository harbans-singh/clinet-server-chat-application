#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#define SERVER_PORT 8080

void receive_messages(int sockfd)
{
    char buffer[1024];
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = read(sockfd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0)
        {
            std::cout << "Disconnected from server.\n";
            break;
        }
        std::cout << buffer << std::flush;
    }
}

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "Connection Failed\n";
        return 1;
    }

    std::string username, password;
    std::cout << "Enter username: ";
    std::getline(std::cin, username);
    std::cout << "Enter password: ";
    std::getline(std::cin, password);

    std::string credentials = username + ":" + password + "\n";
    send(sock, credentials.c_str(), credentials.size(), 0);

    char response[1024];
    memset(response, 0, sizeof(response));
    int bytes = read(sock, response, sizeof(response) - 1);

    if (bytes <= 0)
    {
        std::cerr << "Server closed the connection.\n";
        close(sock);
        return 1;
    }

    std::string server_response(response);

    if (server_response.find("LOGIN_SUCCESS") != std::string::npos)
    {
        std::cout << "Login successful!\n";
    }
    else if (server_response.find("LOGIN_FAILED") != std::string::npos)
    {
        std::cout << "Login failed.\nDo you want to register as a new user? (yes/no): ";
        std::string choice;
        std::cin >> choice;

        if (choice == "yes")
        {
            std::string register_msg = "REGISTER\n";
            send(sock, register_msg.c_str(), register_msg.size(), 0);

            memset(response, 0, sizeof(response));
            read(sock, response, sizeof(response) - 1);

            server_response = response;
            server_response.erase(server_response.find_last_not_of("\n\r") + 1);

            if (std::string(response).find("SEND_NEW_CREDENTIALS") == std::string::npos)
            {
                std::cerr << "Unexpected server response: " << response << std::endl;
                close(sock);
                return 1;
            }

            std::string newCred = username + ":" + password + "\n";
            std::cout << "----New Cred: " << newCred << std::endl;
            send(sock, newCred.c_str(), newCred.size(), 0);

            memset(response, 0, sizeof(response));
            read(sock, response, sizeof(response) - 1);

            if (std::string(response).find("REGISTER_SUCCESS") != std::string::npos)
            {
                std::cout << "Registration successful!\n";
            }
            else
            {
                std::cout << "Registration failed: " << response;
                close(sock);
                return 1;
            }
        }
        else
        {
            std::cout << "Exiting...\n";
            close(sock);
            return 1;
        }
    }
    else
    {
        std::cerr << "Unknown server response: " << server_response << std::endl;
        close(sock);
        return 1;
    }

    std::cout << "Welcome to the chat, " << username << "!\n";

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::thread recv_thread(receive_messages, sock);
    recv_thread.detach();

    std::string input;
    while (std::getline(std::cin, input))
    {
        input += "\n";
        send(sock, input.c_str(), input.size(), 0);
    }

    close(sock);
    return 0;
}
