#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <vector>

#ifdef _MSC_VER
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

constexpr const auto kServerPort = 8080;
constexpr const auto kServerAddress = "127.0.0.1";
constexpr const auto kReciveBufferSize = 1024;

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
    auto connection_result = connect(client_socket_file_descriptor, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address));
    if (connection_result < 0) {
        perror("Failed to connect to server");
        close(client_socket_file_descriptor);
        return EXIT_FAILURE;
    }

    std::cout << "Connected to the server\n";

   std::vector<char> recive_buffer(kReciveBufferSize);
   std::string input_msg;

   for(;;) {
    std::cout << "Enter message to server: " << "\n";
    std::cin >> input_msg;
    auto send_result = send(client_socket_file_descriptor,input_msg.c_str(), input_msg.size(),0);

   }

}