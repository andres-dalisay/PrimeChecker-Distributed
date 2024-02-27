// master_server.cpp
#include <iostream>
#include <winsock2.h>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")


#define THREAD_COUNT 16

std::mutex mtx;
std::vector<int> primes;

bool check_prime(const int& n) {
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

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

void checkPrimeLoop(std::vector<int> checkArray, int start, int end) {
    for (int i = start; i <= end; i++) {
        if (checkArray[i] < 2) continue;
        if (check_prime(checkArray[i])) {
            mtx.lock();
            primes.push_back(checkArray[i]);
            mtx.unlock();
        }
    }
}

void handle_master(std::vector<int> master_task) {
    clock_t start, end;
    start = clock();

    // Create threads
    std::vector<std::thread> threads;
    threads.reserve(THREAD_COUNT);

    int split = master_task.size() / THREAD_COUNT;

    for (int i = 0; i < THREAD_COUNT; i++) {
        int start = i * split;
        int end = (i + 1) * split - 1;

        if (i == THREAD_COUNT - 1) {
			end = master_task.size() - 1;
		}

		threads.emplace_back(checkPrimeLoop, master_task, start, end);
	}

    for (auto& thread : threads) {
        thread.join();
    }

    end = clock();

    double time_taken = double(end - start) / double(CLOCKS_PER_SEC);
    std::cout << "Time taken by program is : " << std::fixed << time_taken << std::setprecision(5);
    
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
        
        //RECEIVE TASK
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

        //SPLIT TASK
        std::vector<int> master_task;
        std::vector<int> slave_task;

        // create a for loop to iterate through the range of numbers
        for (int i = num1; i <= num2; i++) {
            /*if (i % 2) {
				master_task.push_back(i);
            }
            else {
                slave_task.push_back(i);
            }*/
            master_task.push_back(i);
		}

        handle_master(master_task);
        //handle_slave(slave_task);
        

        // Process the task and get the result
        int primesCount = primes.size();
        char result[1024];
        sprintf_s(result, "%d", primesCount);
        send(client_socket, result, strlen(result), 0);

        closesocket(client_socket);

        
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}