// client.cpp
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

// Function to serialize a vector of integers into a byte stream
std::vector<char> serializeVector(const std::vector<int>& vec) {
    std::vector<char> bytes;
    // Assuming integers are 4 bytes each
    for (int num : vec) {
        // Convert each integer to bytes
        char* numBytes = reinterpret_cast<char*>(&num);
        for (size_t i = 0; i < sizeof(num); ++i) {
            bytes.push_back(numBytes[i]);
        }
    }
    return bytes;
}

// Function to deserialize a byte stream into a vector of integers
std::vector<int> deserializeVector(const std::vector<char>& bytes) {
    std::vector<int> vec;
    // Assuming integers are 4 bytes each
    for (size_t i = 0; i < bytes.size(); i += sizeof(int)) {
        int num;
        // Convert bytes back to integer
        memcpy(&num, &bytes[i], sizeof(int));
        vec.push_back(num);
    }
    return vec;
}

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
    std::cout << "Number of primes: " << buffer << std::endl;

    closesocket(client_socket);
    WSACleanup();
}

int main() {
    char start_point[100];
    char end_point[100];

    while (true) {
        std::cout << "Enter start point: ";
        std::cin.getline(start_point, sizeof(start_point));

        std::cout << "Enter end point: ";
        std::cin.getline(end_point, sizeof(end_point));

        try {
            int start_int = std::stoi(start_point);
            int end_int = std::stoi(end_point);

            if (start_int < 0 || end_int < 0 || start_int >= end_int || start_int > 100000000 || end_int > 100000000) {
                std::cerr << "Error: Invalid input. ";
                if (start_int >= end_int) {
                    std::cerr << "Start point must be less than end point. ";
                }
                if (start_int < 0 || end_int < 0) {
                    std::cerr << "Start and end points must be positive. ";
                }
                if (start_int > 100000000 || end_int > 100000000) {
                    std::cerr << "Start and end points must be less than 10^8. ";
                }
                std::cerr << "Please try again." << std::endl;
            }
            else {
                break;
            }
        }
        catch (const std::invalid_argument& e) {
            std::cerr << "Error: Invalid input. Please enter numeric values." << std::endl;
        }
        catch (const std::out_of_range& e) {
			std::cerr << "Error: Input out of range." << std::endl;
		}
    }

    send_task(start_point, end_point);

    return 0;
}