#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <ncurses.h>
#include <mutex>
#include <vector>
#include <sstream>

#define SERVER_PORT 8080

WINDOW *userWin, *chatWin, *inputWin;
std::mutex mtx;
std::vector<std::string> onlineUsers;

void update_user_list()
{
    werase(userWin);
    box(userWin, 0, 0);
    mvwprintw(userWin, 0, 2, " Online Users ");
    int line = 1;
    for (const auto &user : onlineUsers)
    {
        mvwprintw(userWin, line++, 2, "%s", user.c_str());
    }
    wrefresh(userWin);
}

void add_chat_line(const std::string &line)
{
    static int row = 1;
    mtx.lock();
    int max_y, max_x;
    getmaxyx(chatWin, max_y, max_x);
    if (row >= max_y - 1)
    {
        werase(chatWin);
        box(chatWin, 0, 0);
        row = 1;
    }
    mvwprintw(chatWin, row++, 2, "%s", line.c_str());
    wrefresh(chatWin);
    mtx.unlock();
}

void receive_messages(int sockfd)
{
    char buffer[1024];
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = read(sockfd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0)
        {
            add_chat_line("Disconnected from server.");
            break;
        }

        std::string msg(buffer);
        if (msg.find("USER_LIST_UPDATE:") == 0)
        {
            // Example message: USER_LIST_UPDATE:user1,user2,...
            onlineUsers.clear();
            std::string list = msg.substr(msg.find(":") + 1);
            std::istringstream iss(list);
            std::string user;
            while (std::getline(iss, user, ','))
            {
                if (!user.empty())
                    onlineUsers.push_back(user);
            }
            update_user_list();
        }
        else
        {
            add_chat_line(msg);
        }
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

    // std::cout << "Welcome to the chat, " << username << "!\n";

    // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // std::thread recv_thread(receive_messages, sock);
    // recv_thread.detach();

    // std::string input;
    // while (std::getline(std::cin, input))
    // {
    //     input += "\n";
    //     send(sock, input.c_str(), input.size(), 0);
    // }

    initscr();
    cbreak();
    noecho();
    int height, width;
    getmaxyx(stdscr, height, width);

    int userWinWidth = 30;
    int inputWinHeight = 5;
    int chatWinHeight = height - inputWinHeight;
    int chatWinWidth = width - userWinWidth;

    // Correct dimensions
    userWin = newwin(height, userWinWidth, 0, 0);                                 // full-height user window
    chatWin = newwin(chatWinHeight, chatWinWidth, 0, userWinWidth);               // chat window to the right
    inputWin = newwin(inputWinHeight, chatWinWidth, chatWinHeight, userWinWidth); // bottom input window

    keypad(inputWin, TRUE);

    scrollok(chatWin, TRUE);
    box(userWin, 0, 0);
    box(chatWin, 0, 0);
    box(inputWin, 0, 0);
    wrefresh(userWin);
    wrefresh(chatWin);
    wrefresh(inputWin);

    std::thread recv_thread(receive_messages, sock);
    recv_thread.detach();

    char input_buffer[512];

    while (true)
    {
        std::cout << "----In While" << std::endl;
        werase(inputWin);
        box(inputWin, 0, 0);
        mvwgetnstr(inputWin, 1, 2, input_buffer, sizeof(input_buffer) - 1);
        std::string message = input_buffer;
        if (!message.empty())
        {
            message += "\n";
            send(sock, message.c_str(), message.size(), 0);
        }
    }

    close(sock);
    endwin();
    return 0;
}
