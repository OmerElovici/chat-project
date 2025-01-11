#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <vector>

#ifdef _MSC_VER
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

constexpr const auto kServerPort = 8080;
constexpr const auto kServerAddress = "127.0.0.1";
constexpr const auto kReuseOption = 1;
constexpr const auto kReciveBufferSize = 1024;

void client_handle(int client_socket) {
     std::vector<char> recive_buffer(kReciveBufferSize);
     for (;;) {
        auto bytes_received = recv(client_socket, recive_buffer.data(), recive_buffer.size(), 0);

        if (bytes_received > 0) {
            std::cout << "Received from client: " 
                      << std::string(recive_buffer.data(), bytes_received) << "\n";
        } else if (bytes_received == 0) {
            std::cout << "Connection closed by client.\n";
            break;
        } else {
            std::cout << "Error receiving data.\n";
            break;
        }
    }
    close(client_socket);
}

int main(int /*unused*/, char** /*unused*/) {
    // convert host endiness to network endianness which is big endian.
    struct sockaddr_in server_address {
        AF_INET, htons(kServerPort), INADDR_ANY
    };

    // Create TCP socket with format host:port
    auto server_socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_file_descriptor < 0) {
        std::cout << "Failed to create socket\n";
        std::quick_exit(EXIT_FAILURE);
    }

    // configure socket to reuse address and port. (reuse_addr to enssure address:port is availlable to use. reuse_port
    // for load balancing)
    auto setsockopt_result = setsockopt(
      server_socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &kReuseOption, sizeof(kReuseOption));
    if (setsockopt_result != 0) {
        perror("Failed to set server socket!");
        close(server_socket_file_descriptor);
        std::quick_exit(EXIT_FAILURE);
    }

    // require dynamic cast since bind accept  sockaddr types.
    auto* serv_addr_ptr = reinterpret_cast<struct sockaddr*>(&server_address);
    auto bind_result = bind(server_socket_file_descriptor, serv_addr_ptr, sizeof(server_address));
    if (bind_result < 0) {
        std::cout << "Failed to bind socket : " << kServerAddress << ":" << kServerPort << "\n";
        close(server_socket_file_descriptor);
        std::quick_exit(EXIT_FAILURE);
    }

    // Start listening for connections
    auto listen_result = listen(server_socket_file_descriptor, 3);
    if (listen_result < 0) {
        close(server_socket_file_descriptor);
        std::cout << "Failed to start listening for connections\n";
        std::quick_exit(EXIT_FAILURE);
    }

    std::cout << "Waiting for a connection on port " << kServerPort << "...\n";

    // Client connection handling
    struct sockaddr_in client_addr {};
    auto client_len = static_cast<socklen_t>(sizeof(client_addr));
    auto* client_addr_ptr = reinterpret_cast<struct sockaddr*>(&client_addr);
    auto client_socket_file_descriptor = accept(server_socket_file_descriptor, client_addr_ptr, &client_len);
    if (client_socket_file_descriptor < 0) {
        std::cout << "Failed to accept client connection\n";
        close(server_socket_file_descriptor);
        std::quick_exit(EXIT_FAILURE);
    }


    std::vector<char> recive_buffer(kReciveBufferSize);

    for (;;) {
        auto bytes_received = recv(client_socket_file_descriptor, recive_buffer.data(), recive_buffer.size(), 0);

        if (bytes_received > 0) { std::cout << "Recived from client: " << recive_buffer.data() << "\n"; }
        else if (bytes_received == 0) {
            std::cout << "Connection closed.\n";
            break;
        }
        // recive_buffer.clear();
    }

}
