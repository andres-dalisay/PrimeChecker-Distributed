// slave_server.cpp
#include <iostream>
#include <winsock2.h>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <iomanip>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#define MASTER_SERVER_IP "127.0.0.1"


#define THREAD_COUNT std::thread::hardware_concurrency()
#define MAX_BUFFER_SIZE 100000000

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

void handle_slave(std::vector<int> slave_task) {
    // Create threads
    std::vector<std::thread> threads_slave;
    threads_slave.reserve(THREAD_COUNT);

    int split = slave_task.size() / THREAD_COUNT;
    if (split == 0) split = 1;

    for (int i = 0; i < THREAD_COUNT && i < slave_task.size(); i++) {
        int start = i * split;
        int end = (i + 1) * split - 1;

        if (i == THREAD_COUNT - 1) {
            end = slave_task.size() - 1;
        }

        threads_slave.emplace_back(checkPrimeLoop, slave_task, start, end);
    }

    for (auto& thread : threads_slave) {
        thread.join();
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error initializing Winsock" << std::endl;
        return -1;
    }

    SOCKET slave_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (slave_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket" << std::endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in master_address;
    master_address.sin_family = AF_INET;
    master_address.sin_addr.s_addr = INADDR_ANY;
    master_address.sin_port = htons(5001);
    inet_pton(AF_INET, MASTER_SERVER_IP, &master_address.sin_addr);

    if (connect(slave_socket, reinterpret_cast<SOCKADDR*>(&master_address), sizeof(master_address)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket" << std::endl;
        closesocket(slave_socket);
        WSACleanup();
        return -1;
    }

    std::cout << "Slave server is running..." << std::endl;

    std::vector<char> receivedData(MAX_BUFFER_SIZE);
    int bytesReceived = recv(slave_socket, receivedData.data(), receivedData.size(), 0);
    if (bytesReceived == SOCKET_ERROR) {
		std::cerr << "Error receiving data" << std::endl;
	}


    receivedData.resize(bytesReceived);
    std::vector<int> task = deserializeVector(receivedData);
    std::cout << "Received task from client.";

    handle_slave(task);
    std::vector<char> serializedPrimes = serializeVector(primes);

    // print numPrimes
    std::cout << "Number of primes in slave: " << primes.size() << std::endl;
    send(slave_socket, serializedPrimes.data(), serializedPrimes.size(), 0);

    closesocket(slave_socket);
    WSACleanup();
    return 0;
}