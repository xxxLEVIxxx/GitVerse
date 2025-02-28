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

using namespace std;

#define RECEIVE_PORT 21135
#define RESPONSE_PORT 24135

bool isExist(const string &username, const string &filename)
{
    // cout << "The username is " << username << endl;
    ifstream file(filename.c_str());
    string line;

    if (!file.is_open())
    {
        cerr << "Error: Could not open the file." << endl;
        return false;
    }

    while (getline(file, line))
    {
        istringstream ss(line);
        string file_username, file_password;

        // Extract username and password from the line
        ss >> file_username >> file_password;

        // Compare the input username and password with the file data
        if (username == file_username)
        {
            file.close();
            return true; // Authentication success
        }
    }

    file.close();
    return false; // Authentication failure
}

bool authenticate(const string &username, const string &password, const string &filename)
{
    // cout << "Opening file " << filename.c_str() << endl;
    ifstream file(filename.c_str());
    string line;

    if (!file.is_open())
    {
        cerr << "Error opening file!" << endl;
        return false;
    }

    while (getline(file, line))
    {
        istringstream ss(line);
        string file_username, file_password;

        // Extract username and password from the line
        ss >> file_username >> file_password;

        // Compare the input username and password with the file data
        if (username == file_username && password == file_password)
        {
            file.close();
            return true; // Authentication success
        }
    }

    file.close();
    return false; // Authentication failure
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
    std::cout << "Server A is up and running using UDP on port " << RECEIVE_PORT << "..." << std::endl;

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
        string action, username, password;
        iss >> action >> username >> password;
        std::string response;
        if (action == "authenticate")
        {
            // std::cout << "ServerA received username " << username << " and " << password << std::endl;
            std::cout << "ServerA received username " << username << " and ****** " << std::endl;

            if (authenticate(username, password, "./members.txt"))
            {
                response = "auth true";
                cout << "Member " << username << " has been authenticated" << endl;
            }
            else
            {
                response = "auth false";
                cout << "The username " << username << " or password ****** is incorrect" << endl;
            }
        }
        else
        {
            if (isExist(username, "./members.txt"))
            {
                response = "exist true";
            }
            else
            {
                response = "exist false";
            }
        }

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

        // std::cout << response << std::endl;
    }

    close(server_fd);
}

int main()
{
    start_udp_server();
    return 0;
}
