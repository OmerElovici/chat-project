#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ranges>
#include <string_view>
#include <thread>
#include <unistd.h>

#ifdef _MSC_VER
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
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


void print_users(std::string& users_messeges_buffer) {

    std::string_view buffer(users_messeges_buffer);

    for (const auto& part : buffer | std::ranges::views::split(':')) {
        std::cout << "- " << std::string_view(part) << "\n";
    }
}

auto send_msg(int client_socket_file_descriptor, std::string& msg) {

    ssize_t bytes_sent = send(client_socket_file_descriptor, msg.c_str(), msg.size(), 0);
    if (bytes_sent > 0) { std::cout << "Sent: " << msg << " (" << bytes_sent << " bytes)\n"; }
    else { std::cerr << "Error in send\n"; }
}

void server_messages_handler(std::string& server_message) {
    if (server_message.starts_with(kPrivateMsgPrefix)) { std::cout << server_message << "\n"; }
    else if (server_message.starts_with(kGetUsersPrefix)) { print_users(server_message); }
    else if (server_message.starts_with(kServerErrorPrefix)) {
        std::cout << "Server error: " << server_message << "\n";
    }
}

void client_menu(int& client_socket_file_descriptor) {
    std::array<char, kBufferSize> rx_buffer{};
    std::string input;
    std::cout << "Welcome to chat!\n";
    std::cout << "To get list of chatters, enter : " << kGetUsersCommand << "\n";
    std::cout << "To quit the program, enter :  " << kQuitCommand << "\n";
    std::cout << "To send message to user enter: " << kDmCommand << " <user> <msg>\n";
    std::cout << "--------------------------------" << "\n";
    for (;;) {
        std::getline(std::cin, input);
        if (input == kGetUsersCommand) {
            auto bytes_sent = send(client_socket_file_descriptor, kGetUsersCommand, strlen(kGetUsersCommand), 0);
            if (bytes_sent <= 0) { std::cout << "Failed to get users from server\n"; }
        }
        else if (input.starts_with(kDmCommand)) {
            std::string_view view(input);
            auto sent_bytes = send(client_socket_file_descriptor, view.data(), view.size(), 0);
        }
        else if (input == kQuitCommand) {
            auto bytes_sent = send(client_socket_file_descriptor, kQuitCommand, strlen(kQuitCommand), 0);
            std::quick_exit(EXIT_SUCCESS);
        }
    }
}

int main(int /*unused*/, char** /*unused*/) {

    // Create socket file descriptor
    auto client_socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_file_descriptor < 0) {
        std::cout << "Failed to create socket file descriptor\n";
        std::quick_exit(EXIT_FAILURE);
    }


    struct sockaddr_in server_address {
        AF_INET, htons(kServerPort)
    };

    // Convert server address to network binary represantaion
    auto convesion_result = inet_pton(server_address.sin_family, kServerAddress, &server_address.sin_addr);
    if (convesion_result <= 0) {
        perror("Invalid server address");
        close(client_socket_file_descriptor);
        std::quick_exit(EXIT_FAILURE);
    }

    // Connect to server
    auto connection_result = connect(
      client_socket_file_descriptor, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address));
    if (connection_result < 0) {
        perror("Failed to connect to server");
        close(client_socket_file_descriptor);
        return EXIT_FAILURE;
    }

    std::cout << "Connected to the server\n";

    std::string input;
    std::cout << "Enter your userame to server: " << "\n";
    std::cin >> input;
    send_msg(client_socket_file_descriptor, input);

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