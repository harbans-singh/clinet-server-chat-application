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

    // Input username and password
    std::string username, password;
    std::cout << "Enter username: ";
    std::cin >> username;
    std::cout << "Enter password: ";
    std::cin >> password;

    std::string credentials = username + ":" + password + "\n";
    send(sock, credentials.c_str(), credentials.size(), 0);

    // Wait for server response
    char response[1024];
    memset(response, 0, sizeof(response));
    read(sock, response, sizeof(response));

    if (std::string(response).find("LOGIN_SUCCESS") == std::string::npos)
    {
        std::cout << response;

        std::string userChoice;
        std::cout << "User not found!\nDo you want to register as a new User?(yes/no)\n"
                  << std::endl;
        std::cin >> userChoice;
        if (userChoice == "yes")
        {
            std::string registerRequest = "REGISTER\n";
            send(sock, registerRequest.c_str(), registerRequest.size(), 0);

            memset(response, 0, sizeof(response));
            read(sock, response, sizeof(response));

            if (std::string(response).find("REGISTER_SUCCESS") != std::string::npos)
            {
                std::cout << "Registered and logged in successfully!\n";
            }
            else
            {
                std::cout << "Registration failed: " << response << std::endl;
                close(sock);
                return 1;
            }
        }
        else
        {
            std::cout << "Registration aborted, closing connection..." << std::endl;
            close(sock);
            return 1;
        }
    }

    std::cout << "Welcome to the chat, " << username << "!\n";

    // Start receiving thread
    std::thread recv_thread(receive_messages, sock);
    recv_thread.detach();

    // Sending loop
    std::string input;
    while (std::getline(std::cin, input))
    {
        input += "\n";
        send(sock, input.c_str(), input.size(), 0);
    }

    close(sock);
    return 0;
}
