#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>  // For perror
#include <cstdlib> // For exit()
#include <fstream>
#include <vector>
using namespace std;

#define RECEIVE_PORT 23135
#define RESPONSE_PORT 24135

void writeToDeployedFile(const string &data, const string &outputFile)
{
    istringstream iss(data);
    string command, username, filename;

    // Parse the first word to check if it's the "deploy" command
    iss >> command;
    if (command != "deploy")
    {
        cerr << "Error: Invalid data format. Expected 'deploy' command." << endl;
        return;
    }

    // Collect entries from the data string
    vector<pair<string, string>> entries;
    while (iss >> username >> filename)
    {
        entries.emplace_back(username, filename);
    }

    // Check if there are any entries
    if (entries.empty())
    {
        cout << "No entries available to write to " << outputFile << endl;
        return;
    }

    // Open the file for writing
    ofstream file(outputFile);
    if (!file.is_open())
    {
        cerr << "Error: Could not open the file for writing." << endl;
        return;
    }

    // Write the header line
    file << "UserName Filename" << endl;

    // Write each entry
    for (const auto &entry : entries)
    {
        file << entry.first << " " << entry.second << endl;
    }

    file.close(); // Close the file
    cout << "Server D has deployed the user " << username << "\'s repository." << endl;
}

void start_udp_server()
{
    int server_fd;
    struct sockaddr_in receive_addr, client_addr, response_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create UDP socket
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
    {
        perror("Socket failed");
        return;
    }

    // Configure receive address
    receive_addr.sin_family = AF_INET;
    receive_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
    receive_addr.sin_port = htons(RECEIVE_PORT);

    // Bind the socket to the receive port
    if (bind(server_fd, (struct sockaddr *)&receive_addr, sizeof(receive_addr)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        return;
    }

    // std::cout << "UDP Server is listening on port " << RECEIVE_PORT << "..." << std::endl;
    std::cout << "Server D is up and running using UDP on port " << RECEIVE_PORT << "..." << std::endl;

    char buffer[1024];

    while (true)
    {
        // Receive message from client
        int len = recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (len < 0)
        {
            perror("Receive failed");
            break;
        }

        buffer[len] = '\0'; // Null-terminate the received string

        // Parse the request
        istringstream iss(buffer);
        string action, param;
        iss >> action >> param;
        // std::cout << "ServerA received username " << username << " and " << password << std::endl;
        // std::cout << "ServerA received username " << param << " and ****** " << std::endl;
        if (action == "deploy")
        {
            cout << "Server D has received a deploy request from the main server." << endl;
            writeToDeployedFile(string(buffer), "deployed.txt");
        }
        std::string response = std::string(buffer);

        // Configure response address (server sends from RESPONSE_PORT)
        response_addr.sin_family = AF_INET;
        response_addr.sin_addr.s_addr = INADDR_ANY; // Local host
        response_addr.sin_port = htons(RESPONSE_PORT);

        // Send response back to the client
        if (sendto(server_fd, response.c_str(), response.size(), 0, (struct sockaddr *)&client_addr, client_addr_len) < 0)
        {
            perror("Send failed");
            break;
        }
    }

    close(server_fd);
}

int main()
{
    start_udp_server();
    return 0;
}