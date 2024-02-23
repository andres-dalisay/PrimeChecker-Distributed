// master_server.cpp
#include <iostream>
#include <winsock2.h>
#include <sstream>
#include <string>
#include <vector>
#include <thread>

#pragma comment(lib, "ws2_32.lib")


#define THREAD_COUNT 16

std::vector<int> primes;

bool check_prime(const int& n) {
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

void checkPrimeLoop(int start, int end) {
    for (int i = start; i <= end; i++) {
        if (i < 2) continue;
        if (check_prime(i)) {
            primes.push_back(i);
        }
    }
}

void handle_client(SOCKET client_socket) {
    char buffer[1024] = { 0 };
    recv(client_socket, buffer, sizeof(buffer), 0);

    std::string strBuffer(buffer); 
    std::stringstream ss(strBuffer);
    std::string token;

                                
    getline(ss, token, ',');     // Get the first number before comma
    int num1 = std::stoi(token); // Convert string to integer

    
    getline(ss, token);          // Get the second number after comma
    int num2 = std::stoi(token); // Convert string to integer

    std::cout << "Received task from client: " << num1 << num2 << std::endl;

    // Create threads
    std::vector<std::thread> threads;
    threads.reserve(THREAD_COUNT);

    int split = num2 / THREAD_COUNT;

    for (int i = 0; i < THREAD_COUNT; i++) {
		int start = i * split;
		int end = (i + 1) * split - 1;
        if (i == THREAD_COUNT - 1) {
			end = num2;
		}
		threads.emplace_back(checkPrimeLoop, start, end);
	}

    for (auto& thread : threads) {
        thread.join();
    }


    // Process the task and get the result
    const char* result = "Task completed";
    send(client_socket, result, strlen(result), 0);

    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error initializing Winsock" << std::endl;
        return -1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket" << std::endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(5000);

    if (bind(server_socket, reinterpret_cast<SOCKADDR*>(&server_address), sizeof(server_address)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR) {
        std::cerr << "Error listening on socket" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    std::cout << "Master server is running..." << std::endl;

    while (true) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }

        handle_client(client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}