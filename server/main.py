import socket
import threading
import select
import sys

from status_codes import ServerStatusCode


class ChatServer:

    msg_prefix = "--"
    error_prefix = "!!"
    info_prefix = "ii"
    user_list_prefix = "++"

    def __init__(self, host="127.0.0.1", port=8080, max_clients=5):
        self.host = host
        self.port = port
        self.max_clients = max_clients
        self.server_socket = None
        self.clients = (
            {}
        )  # Dictionary to store client sockets and usernames -- TODO: use db later
        self.input_list = []

    def start(self):
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(self.max_clients)
        self.input_list.append(self.server_socket)

        print(f"Chat server started on {self.host}:{self.port}")

        try:
            while True:
                read_sockets, _, error_sockets = select.select(
                    self.input_list, [], self.input_list
                )

                for notified_socket in read_sockets:
                    if notified_socket == self.server_socket:
                        self.accept_new_client()
                    else:
                        self.handle_client_message(notified_socket)

                for notified_socket in error_sockets:
                    self.remove_client(notified_socket)

        except KeyboardInterrupt:
            print("Shutting down the server.")
        finally:
            self.shutdown()

    def accept_new_client(self):
        client_socket, client_address = self.server_socket.accept()
        self.input_list.append(client_socket)

        try:
            username = client_socket.recv(1024).decode().strip()
            if not username:
                self.remove_client(client_socket)
                return

            while username in self.clients.values():
                client_socket.send(ServerStatusCode.USERNAME_TAKEN.value.encode())
                username = client_socket.recv(1024).decode().strip()
                if not username:
                    self.remove_client(client_socket)
                    return

            self.clients[client_socket] = username
            print(f" >> {username} has joined the chat room")
            client_socket.send(ServerStatusCode.SUCCESS.value.encode())
            # self.broadcast(
            #     f"{self.info_prefix} {username} has joined the chat!", client_socket
            # )

        except (ConnectionResetError, BrokenPipeError):
            print(f" >> Client {client_address} disconnected during username setup")
            self.remove_client(client_socket)

    def handle_client_message(self, client_socket):

        user = self.clients[client_socket]

        try:
            message = client_socket.recv(1024).decode()
            if message:
                if message.lower() == "/quit":
                    self.remove_client(client_socket)
                elif message.lower().startswith("/dm"):
                    self.handle_direct_message(client_socket, message)
                elif message.lower().startswith("/b"):
                    print(f" >> {user} sent broadcast messege: {message}")
                    self.broadcast(message, client_socket)
                elif message.lower().startswith("/users"):
                    connected_clients = self.clients.values()
                    separator = ":"
                    response = self.user_list_prefix + separator.join(
                        map(str, connected_clients)
                    )
                    print(f" >> '{user}' asked for user list ( {response} )")
                    client_socket.send(response.encode())
            else:
                self.remove_client(client_socket)
        except (ConnectionResetError, BrokenPipeError):
            print(
                f" >> Client {self.clients.get(client_socket, 'unknown')} disconnected abruptly."
            )
            self.remove_client(client_socket)

    def handle_direct_message(self, sender_socket, message):
        try:
            parts = message.split(" ", 2)
            recipient_username = parts[1]
            message = parts[2]

            recipient_socket = None
            for sock, username in self.clients.items():
                if username == recipient_username:
                    recipient_socket = sock
                    break

            if recipient_socket:
                print(
                    f" >> dm from {self.clients[sender_socket]} to {self.clients[recipient_socket]}: {message}"
                )
                recipient_socket.send(
                    f"{self.msg_prefix} Recieved direct message from {self.clients[sender_socket]}: {message}".encode()
                )
            else:
                sender_socket.send(ServerStatusCode.USER_NOT_FOUND.value.encode())
        except IndexError:
            sender_socket.send(ServerStatusCode.INVALID_MESSAGE_FORMAT.value.encode())

    def broadcast(self, message, sender_socket):

        if message.startswith("/b"):
            message = f"{self.msg_prefix} Recieved broadcast message from {self.clients[sender_socket]}: {message[3:]}"
        print(f" >> Broadcasting: {message}")
        for client_socket in self.clients:
            if client_socket != sender_socket:
                try:
                    client_socket.send(message.encode())
                except (ConnectionResetError, BrokenPipeError):
                    print(
                        f" >> Failed to broadcast message to {self.clients[client_socket]}."
                    )
                    self.remove_client(client_socket)

    def remove_client(self, client_socket):
        if client_socket in self.clients:
            username = self.clients[client_socket]
            print(f" >> {username} has left the chat.")
            self.broadcast(
                f"{self.info_prefix} {username} has left the chat.", client_socket
            )
            del self.clients[client_socket]
        if client_socket in self.input_list:
            self.input_list.remove(client_socket)
            client_socket.close()

    def shutdown(self):
        self.broadcast(f"{self.info_prefix} Server is shutting down.", None)
        for client_socket in self.clients:
            client_socket.close()
        self.server_socket.close()


if __name__ == "__main__":
    server = ChatServer()
    server.start()
