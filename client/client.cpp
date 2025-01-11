#include <array>
#include <cstdlib>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
constexpr static void close_socket(socket_t sock) { closesocket(sock); }
static void init_win_socket() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WSAStartup failed\n";
        std::quick_exit(EXIT_FAILURE);
    }
}
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
constexpr static void close_socket(socket_t sock) { close(sock); }
#endif

constexpr const auto kServerPort = 8080;
constexpr const auto kServerAddress = "127.0.0.1";
constexpr const auto kBufferSize = 1024;

constexpr const auto kGetUsersCommand = "/users";
constexpr const auto kQuitCommand = "/quit";
constexpr const auto kDmCommand = "/dm";

constexpr const auto kPrivateMsgPrefix = "--";
constexpr const auto kServerErrorPrefix = "!!";
constexpr const auto kGetUsersPrefix = "++";
constexpr const auto kServerInfoPrefix = "ii";
constexpr const auto kSuccessResponsePrefix = "200";

static void print_users(std::string& users_messeges_buffer) {
    users_messeges_buffer.erase(0, strlen(kGetUsersPrefix));
    std::cout << users_messeges_buffer << "\n";
    std::string_view buffer(users_messeges_buffer);

    for (const auto& part : buffer | std::ranges::views::split(':')) {
        std::cout << "- " << std::string_view(part) << "\n";
    }
}


static void send_msg(socket_t client_socket_file_descriptor, std::string& msg) {
    auto bytes_sent = send(client_socket_file_descriptor, msg.data(), static_cast<int>(msg.length()), 0);
    if (bytes_sent > 0) { std::cout << "Sent: " << msg << "\n"; }
    else { std::cout << "Failed to send:" << msg << "\n"; }
}
static void send_command(socket_t client_socket_file_descriptor, const char* command) {
    auto bytes_sent = send(client_socket_file_descriptor, command, sizeof(command), 0);
    if (bytes_sent > 0) { std::cout << "Sent: " << command << "\n"; }
    else { std::cout << "Failed to send:" << command << "\n"; }
}


static int server_messages_handler(std::string& server_message) {

    if (server_message.starts_with(kPrivateMsgPrefix)) {
        std::cout << " ";
        std::cout << server_message << "\n";
    }
    else if (server_message.starts_with(kGetUsersPrefix)) { print_users(server_message); }
    else if (server_message.starts_with(kServerErrorPrefix)) {
        std::cout << "Server error: " << server_message << "\n";
    }
    else if (server_message.starts_with(kSuccessResponsePrefix)) { return 0; }
    return 0;
}

static void client_menu(socket_t client_socket_file_descriptor) {
    std::array<char, kBufferSize> rx_buffer{};
    std::string input;
    std::cout << "Welcome to chat!\n";
    std::cout << "To get list of chatters, enter : " << kGetUsersCommand << "\n";
    std::cout << "To quit the program, enter :  " << kQuitCommand << "\n";
    std::cout << "To send message to user enter: " << kDmCommand << " <user> <msg>\n";
    std::cout << "--------------------------------" << "\n";
    for (;;) {
        std::getline(std::cin, input);
        if (input == kGetUsersCommand) { send_command(client_socket_file_descriptor, kGetUsersCommand); }
        else if (input.starts_with(kDmCommand)) {
            std::string_view view(input);
            send_msg(client_socket_file_descriptor, input);
        }
        else if (input == kQuitCommand) {
            send_command(client_socket_file_descriptor, kQuitCommand);
            std::quick_exit(EXIT_SUCCESS);
        }
    }
}

int main(int /*unused*/, char** /*unused*/) {
#ifdef _WIN32
    init_win_socket();
#endif
    // Create socket file descriptor
    auto client_socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_file_descriptor < 0) {
        std::cout << "Failed to create socket file descriptor\n";
        std::quick_exit(EXIT_FAILURE);
    }
#ifdef _WIN32
#endif

    struct sockaddr_in server_address{ .sin_family = AF_INET, .sin_port = htons(kServerPort) };

    // Convert server address to network binary represantaion
    auto convesion_result = inet_pton(server_address.sin_family, kServerAddress, &server_address.sin_addr);
    if (convesion_result <= 0) {
        std::cout << "Invalid server address\n";
        close_socket(client_socket_file_descriptor);
        std::quick_exit(EXIT_FAILURE);
    }

    // Connect to server
    auto connection_result = connect(
      client_socket_file_descriptor, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address));
    if (connection_result < 0) {
        std::cout << "Failed to connect to server\n";
        close_socket(client_socket_file_descriptor);
        std::quick_exit(EXIT_FAILURE);
    }

    std::cout << "Connected to the server\n";

    std::string input;
    std::array<char, kBufferSize> recieve_buffer{};
    auto response = -1;
    while (response == -1) {
        std::cout << "Enter your userame to server: " << "\n";
        std::getline(std::cin, input);
        if (input.length() > 0) {
            send_msg(client_socket_file_descriptor, input);
            auto recieved_bytes = recv(client_socket_file_descriptor, recieve_buffer.data(), recieve_buffer.size(), 0);
            std::string recv_msg = recieve_buffer.data();
            if (recieved_bytes > 0) { response = server_messages_handler(recv_msg); }
        }
    }

    std::jthread receive_loop([client_socket_file_descriptor] {
        std::array<char, kBufferSize> msg{};
        for (;;) {
            auto recieved_bytes = recv(client_socket_file_descriptor, msg.data(), msg.size(), 0);
            if (recieved_bytes > 0) {
                std::string receive_message = msg.data();
                server_messages_handler(receive_message);
            }
        }
    });

    client_menu(client_socket_file_descriptor);
}