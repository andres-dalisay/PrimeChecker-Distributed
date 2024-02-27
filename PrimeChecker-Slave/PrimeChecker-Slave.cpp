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


#define THREAD_COUNT 16
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
    clock_t start, end;
    start = clock();

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
    inet_pton(AF_INET, "127.0.0.1", &master_address.sin_addr);

    if (connect(slave_socket, reinterpret_cast<SOCKADDR*>(&master_address), sizeof(master_address)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket" << std::endl;
        closesocket(slave_socket);
        WSACleanup();
        return -1;
    }

    //if (listen(server_socket, 5) == SOCKET_ERROR) {
    //    std::cerr << "Error listening on socket" << std::endl;
    //    closesocket(server_socket);
    //    WSACleanup();
    //    return -1;
    //}

    std::cout << "Slave server is running..." << std::endl;

    while (true) {

        std::vector<char> receivedData(MAX_BUFFER_SIZE);
        int bytesReceived = recv(slave_socket, receivedData.data(), receivedData.size(), 0);
        if (bytesReceived == SOCKET_ERROR) {
			std::cerr << "Error receiving data" << std::endl;
            continue;
		}


        receivedData.resize(bytesReceived);
        std::vector<int> task = deserializeVector(receivedData);
        std::cout << "Received task from client.";

        handle_slave(task);
        std::vector<int> numPrimes;
        numPrimes.push_back(primes.size());

        // print numPrimes
        std::cout << "Number of primes in slave: " << numPrimes.front() << std::endl;
        // convert numPrimes to bytes
        std::vector<char> numBytes = serializeVector(numPrimes);
        for (int i = 0; i < numBytes.size(); i++) {
			std::cout << numBytes[i];
		}
        send(slave_socket, numBytes.data(), numBytes.size(), 0);

        //handle_client(master_socket);
    }

    closesocket(slave_socket);
    WSACleanup();
    return 0;
}