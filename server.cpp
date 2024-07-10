#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <unordered_map>
#include <cstring>

#ifdef _WIN32
#include <Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

const int PORT = 8080;
const int MAX_CLIENTS = 10;

std::vector<SOCKET> clients;
std::unordered_map<SOCKET, std::string> client_usernames;
std::mutex clients_mutex;

std::unordered_map<std::string, std::string> user_credentials = {
    {"user1", "password1"},
    {"user2", "password2"},
    {"user3", "password3"},
    {"user4", "password4"}
};

void broadcastMessage(const std::string& message, SOCKET sender) {
    std::lock_guard<std::mutex> guard(clients_mutex);
    for (SOCKET client : clients) {
        if (client != sender) {
            send(client, message.c_str(), message.size(), 0);
        }
    }
}

void handleClient(SOCKET client_socket) {
    char buffer[1024];
    while (true) {
        // Receive message from client
        int bytes_received = recv(client_socket, buffer, 1024, 0);
        if (bytes_received <= 0) {
            // Client disconnected
#ifdef _WIN32
            closesocket(client_socket);
#else
            close(client_socket);
#endif
            std::lock_guard<std::mutex> guard(clients_mutex);
            // Remove client from vector
            auto it = std::find(clients.begin(), clients.end(), client_socket);
            if (it != clients.end()) {
                clients.erase(it);
            }
            std::cout << "Client disconnected" << std::endl;
            break;
        }

        // Broadcast message to all other clients
        std::string message(buffer, bytes_received);
        broadcastMessage(message, client_socket);
        std::cout << "Received: " << message << std::endl;
    }
}


bool authenticateClient(SOCKET client_socket) {
    char buffer[1024];

    // Receive user ID
    int bytes_received = recv(client_socket, buffer, 1024, 0);
    if (bytes_received <= 0) return false;
    std::string user_id(buffer, bytes_received);

    // Receive password
    bytes_received = recv(client_socket, buffer, 1024, 0);
    if (bytes_received <= 0) return false;
    std::string password(buffer, bytes_received);

    if (user_credentials.find(user_id) != user_credentials.end() && user_credentials[user_id] == password) {
        std::string welcome_message = "Welcome, " + user_id;
        send(client_socket, welcome_message.c_str(), welcome_message.size(), 0);
        client_usernames[client_socket] = user_id;
        return true;
    } else {
        std::string failure_message = "Authentication failed";
        send(client_socket, failure_message.c_str(), failure_message.size(), 0);
        return false;
    }
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        std::cerr << "WSAStartup failed: " << wsaResult << std::endl;
        return 1;
    }
#endif

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
#ifdef _WIN32
        closesocket(server_socket);
        WSACleanup();
#else
        close(server_socket);
#endif
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
#ifdef _WIN32
        closesocket(server_socket);
        WSACleanup();
#else
        close(server_socket);
#endif
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        if (!authenticateClient(client_socket)) {
#ifdef _WIN32
            closesocket(client_socket);
#else
            close(client_socket);
#endif
            continue;
        }

        {
            std::lock_guard<std::mutex> guard(clients_mutex);
            clients.push_back(client_socket);
        }

        std::cout << "Client connected" << std::endl;

        std::thread(handleClient, client_socket).detach();
    }

#ifdef _WIN32
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif
    return 0;
}
