#define UNICODE // Define UNICODE to use InetPtonA
#include <iostream>
#include <string>
#include <thread>
#include <Winsock2.h>
#include <Ws2tcpip.h> // For InetPtonA
#pragma comment(lib, "Ws2_32.lib")

const std::string serverIP = "127.0.0.1"; // ANSI string for IP
const int PORT = 8080;

void receiveMessages(SOCKET client_socket) {
    char buffer[1024];
    int bytes_received;

    while (true) {
        // Receive message from server
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cerr << "Server disconnected or communication error" << std::endl;
            break;
        }
        buffer[bytes_received] = '\0';
        std::cout << buffer << std::endl;
    }
}

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    // Create socket
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Prepare sockaddr_in structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert ANSI IP address to IPv4 format
    if (InetPtonA(AF_INET, serverIP.c_str(), &server_addr.sin_addr) != 1) {
        std::cerr << "Invalid address format" << std::endl;
        return 1;
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Connect failed" << std::endl;
        return 1;
    }

    std::cout << "Connected to server" << std::endl;

    // User authentication
    std::string user_id, password;
    std::cout << "Enter user ID: ";
    std::getline(std::cin, user_id);
    send(client_socket, user_id.c_str(), user_id.size(), 0);

    std::cout << "Enter password: ";
    std::getline(std::cin, password);
    send(client_socket, password.c_str(), password.size(), 0);

    char buffer[1024];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) {
        std::string response(buffer, bytes_received);
        if (response == "Authentication failed") {
            std::cerr << "Authentication failed" << std::endl;
            return 1;
        } else {
            std::cout << response << std::endl;
        }
    }

    // Start thread to receive messages from server
    std::thread(receiveMessages, client_socket).detach();

   // Send and receive data with server as needed
while (true) {
    std::string message;
    std::cout << " ";
    std::getline(std::cin, message);

    if (message == "quit")
        break;

    // Format message with username
    std::string full_message = user_id + ": " + message;

    // Send message to server
    int bytes_sent = send(client_socket, full_message.c_str(), full_message.size(), 0);
    if (bytes_sent == -1) {
        std::cerr << "Failed to send message" << std::endl;
        break;
    }
}


      

    // Cleanup Winsock
    WSACleanup();

    return 0;
}
