// client.cpp
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

void send_task(const char* start_point, const char* end_point) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error initializing Winsock" << std::endl;
        return;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket" << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    if (connect(client_socket, reinterpret_cast<SOCKADDR*>(&server_address), sizeof(server_address)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to server" << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return;
    }

    char task[256];
    snprintf(task, sizeof(task), "%s,%s", start_point, end_point);
    send(client_socket, task, strlen(task), 0);

    char buffer[1024] = { 0 };
    recv(client_socket, buffer, sizeof(buffer), 0);
    std::cout << "Result from master server: " << buffer << std::endl;

    closesocket(client_socket);
    WSACleanup();
}

int main() {
    char start_point[100];
    char end_point[100];

    std::cout << "Enter start point: ";
    std::cin.getline(start_point, sizeof(start_point));

    std::cout << "Enter end point: ";
    std::cin.getline(end_point, sizeof(end_point));

    send_task(start_point, end_point);

    return 0;
}