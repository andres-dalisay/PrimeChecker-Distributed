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
    if(split == 0) split = 1;

    for (int i = 0; i < THREAD_COUNT && i < master_task.size(); i++) {
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

    // Create a socket for clients
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Client socket creation failed." << std::endl;
        WSACleanup();
        return 1;
    }

    // Create a socket for slave servers
    SOCKET slaveSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (slaveSocket == INVALID_SOCKET) {
        std::cout << "Slave socket creation failed." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Bind the client socket to an address and port
    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(5000); // Port number
    clientAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address

    if (bind(clientSocket, reinterpret_cast<sockaddr*>(&clientAddr), sizeof(clientAddr)) == SOCKET_ERROR) {
        std::cout << "Client socket bind failed." << std::endl;
        closesocket(clientSocket);
        closesocket(slaveSocket);
        WSACleanup();
        return 1;
    }

    // Bind the slave socket to an address and port
    sockaddr_in slaveAddr;
    slaveAddr.sin_family = AF_INET;
    slaveAddr.sin_port = htons(5001); // Port number for slave connections
    slaveAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address

    if (bind(slaveSocket, reinterpret_cast<sockaddr*>(&slaveAddr), sizeof(slaveAddr)) == SOCKET_ERROR) {
        std::cout << "Slave socket bind failed." << std::endl;
        closesocket(clientSocket);
        closesocket(slaveSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections on the client socket
    if (listen(clientSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Client socket listen failed." << std::endl;
        closesocket(clientSocket);
        closesocket(slaveSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections on the slave socket
    if (listen(slaveSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Slave socket listen failed." << std::endl;
        closesocket(clientSocket);
        closesocket(slaveSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Master server is running..." << std::endl;

    while (true) {
        SOCKET client_socket = accept(clientSocket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }
        SOCKET slave_socket = accept(slaveSocket, NULL, NULL);
        if (slave_socket == INVALID_SOCKET) {
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
            if (i % 2) {
				master_task.push_back(i);
            }
            else {
                slave_task.push_back(i);
            }
            //master_task.push_back(i);
		}

        std::vector<char> slave_task_bytes = serializeVector(slave_task);
        send(slave_socket, slave_task_bytes.data(), slave_task_bytes.size(), 0);
        std::cout << "Sent task to slave server" << std::endl;

        handle_master(master_task);
        //send(client_socket, serializeVector(primes).data(), serializeVector(primes).size(), 0);
        //handle_slave(slave_task);
        

        // Process the task and get the result
        int primesCount = primes.size();
        char result[1024];
        sprintf_s(result, "%d", primesCount);
        //send(client_socket, result, strlen(result), 0);

        closesocket(client_socket);

        
    }

    closesocket(clientSocket);
    closesocket(slaveSocket);
    WSACleanup();
    return 0;
}