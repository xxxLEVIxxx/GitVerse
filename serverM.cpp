#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdio>  // For perror
#include <cstdlib> // For exit()

using namespace std;

mutex shared_mutex; // Mutex to protect shared state

#define MAX_CLIENTS 100
#define MAX_COMMANDS 100
#define MAX_COMMAND_LENGTH 256
#define HISTORY_BUFFER_SIZE 4096 // Buffer size for storing the history string

struct CommandHistory
{
    char username[50];
    char commands[MAX_COMMANDS][MAX_COMMAND_LENGTH];
    int command_count;
};

struct CommandHistory history[MAX_CLIENTS];
int client_count = 0;

// Function to extract the command (remove the username)
void extract_command(const char *input, char *command, char *username)
{
    const char *last_space = strrchr(input, ' ');
    if (last_space)
    {
        // Extract username (after the last space)
        strcpy(username, last_space + 1);
        // Extract command (everything before the last space)
        strncpy(command, input, last_space - input);
        command[last_space - input] = '\0'; // Null-terminate the command string
    }
    else
    {
        strcpy(command, input); // Assume entire input is the command
        username[0] = '\0';     // No username
    }
}

// Store the command for a specific user
void store_command(const char *input)
{
    char command[MAX_COMMAND_LENGTH];
    char username[50];
    int i;

    // Extract the command and username
    extract_command(input, command, username);

    // Check if the user already exists
    for (i = 0; i < client_count; i++)
    {
        if (strcmp(history[i].username, username) == 0)
        {
            // Add the command to the existing user's history
            if (history[i].command_count < MAX_COMMANDS)
            {
                strcpy(history[i].commands[history[i].command_count], command);
                history[i].command_count++;
            }
            return;
        }
    }

    // If user not found, create a new entry
    if (client_count < MAX_CLIENTS)
    {
        strcpy(history[client_count].username, username);
        strcpy(history[client_count].commands[0], command);
        history[client_count].command_count = 1;
        client_count++;
    }
    else
    {
        // printf("Maximum client limit reached. Cannot store command.\n");
    }
}

// Return the command history for a specific user as a formatted string
char *print_history(const char *username)
{
    static char history_buffer[HISTORY_BUFFER_SIZE]; // Static buffer for the output string
    history_buffer[0] = '\0';                        // Clear the buffer

    int i, j;
    for (i = 0; i < client_count; i++)
    {
        if (strcmp(history[i].username, username) == 0)
        {
            for (j = 0; j < history[i].command_count; j++)
            {
                char entry[MAX_COMMAND_LENGTH + 10]; // Temporary buffer for each entry
                snprintf(entry, sizeof(entry), "%d. %s\n", j + 1, history[i].commands[j]);
                strncat(history_buffer, entry, sizeof(history_buffer) - strlen(history_buffer) - 1);
            }
            return history_buffer; // Return the formatted string
        }
    }
    snprintf(history_buffer, sizeof(history_buffer), "No history found for %s.\n", username);
    return history_buffer;
}

// Function to handle UDP request and response
void handle_udp_request(const string &udp_request, const string &udp_server_ip, int udp_server_port, string &udp_response)
{
    int udp_socket;
    struct sockaddr_in udp_server_addr, udp_client_addr;
    char buffer[1024];

    // Create UDP socket
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    // bind the socket to the client
    memset(&udp_client_addr, 0, sizeof(udp_client_addr));
    udp_client_addr.sin_family = AF_INET;
    udp_client_addr.sin_addr.s_addr = INADDR_ANY;
    udp_client_addr.sin_port = htons(24135);

    if (bind(udp_socket, (struct sockaddr *)&udp_client_addr, sizeof(udp_client_addr)) < 0)
    {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }

    memset(&udp_server_addr, 0, sizeof(udp_server_addr));
    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_port = htons(udp_server_port);
    if (inet_pton(AF_INET, udp_server_ip.c_str(), &udp_server_addr.sin_addr) <= 0)
    {
        perror("Invalid UDP server IP address");
        exit(EXIT_FAILURE);
    }

    // Send the UDP request
    if (sendto(udp_socket, udp_request.c_str(), udp_request.length(), 0, (struct sockaddr *)&udp_server_addr, sizeof(udp_server_addr)) < 0)
    {
        perror("UDP send failed");
        exit(EXIT_FAILURE);
    }

    // cout << "UDP request sent!" << endl;

    // Receive the UDP response
    socklen_t addr_len = sizeof(udp_server_addr);
    int n = recvfrom(udp_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&udp_server_addr, &addr_len);
    if (n < 0)
    {
        perror("UDP receive failed");
        exit(EXIT_FAILURE);
    }

    buffer[n] = '\0';              // Null terminate the response
    udp_response = string(buffer); // Store the response

    // cout << "UDP response received: " << udp_response << endl;

    close(udp_socket);
}

// TCP server function that handles client requests
void tcp_server(int tcp_port, const string &udp_server_ip, int udp_port)
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024];
    string udp_response;

    // Create TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("TCP socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(tcp_port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("TCP bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("TCP listen failed");
        exit(EXIT_FAILURE);
    }

    cout << "Server M is up and running using UDP on port " << udp_port << "." << endl;

    // Accept incoming TCP client connections
    while (true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("TCP accept failed");
            continue;
        }

        // cout << "New TCP client connected!" << endl;

        // Read the request from the TCP client
        int valread = read(new_socket, buffer, sizeof(buffer));
        if (valread < 0)
        {
            perror("TCP read failed");
            continue;
        }

        buffer[valread] = '\0'; // Null-terminate the request string
        // cout << "Received TCP request: " << buffer << endl;
        store_command(buffer);

        // Parse the request
        istringstream iss(buffer);
        string action, param;
        iss >> action >> param;

        int target_port;

        if (action == "authenticate")
        {
            target_port = 21135;
            cout << "Server M has received username " << param << " and password ****." << endl;
        }
        else if (action == "push" || action == "lookup" || action == "overwrite" || action == "remove")
        {
            target_port = 22135;
            string username;
            iss >> username;
            if (action == "lookup")
            {

                if (username == "guest")
                {
                    username = "Guest";
                }
                string new_buffer = "exist " + param;
                thread udp_thread(handle_udp_request, new_buffer, udp_server_ip, 21135, ref(udp_response));
                udp_thread.join();
                if (udp_response == "exist false")
                {
                    cout << "The main server has received a lookup request from " << username << " to lookup " << param << "\'s repository using TCP over port " << tcp_port << "." << endl;
                    send(new_socket, "lookup -1", 9, 0);
                    cout << "The main server has sent the response to the client." << endl;
                    close(new_socket);
                    continue;
                }

                cout << "The main server has received a lookup request from " << username << " to lookup " << param << "\'s repository using TCP over port " << tcp_port << "." << endl;
            }
            else if (action == "push")
            {
                cout << "The main server has received a push request from " << username << " using TCP over port " << tcp_port << "." << endl;
            }
            else if (action == "overwrite")
            {
                iss >> username;
                cout << "The main server has received the overwrite confirmation response from " << username << " using TCP over " << tcp_port << endl;
            }
            else if (action == "remove")
            {
                cout << "The main server has received a remove request from member " << username << " using TCP over port " << tcp_port << "." << endl;
            }
        }
        else if (action == "deploy")
        {
            cout << "The main server has received a deploy request from " << param << " using TCP over port " << tcp_port << "." << endl;
            cout << "The main server has sent the lookup request to server R." << endl;
            thread udp_thread(handle_udp_request, string(buffer), udp_server_ip, 22135, ref(udp_response));
            udp_thread.join();
            cout << "The main server received the lookup response from server R." << endl;
            strcpy(buffer, udp_response.c_str());
            target_port = 23135;
        }
        else if (action == "log")
        {
            cout << "The main server has received a log request from member " << param << " using TCP over port " << tcp_port << "." << endl;
            char *record = print_history(param.c_str());
            string record_str(record);
            send(new_socket, record_str.c_str(), record_str.length(), 0);
            cout << "The main server has sent the log response to client." << endl;
            close(new_socket);
            continue;
        }

        // Handle the UDP request in a separate thread
        thread udp_thread(handle_udp_request, string(buffer), udp_server_ip, target_port, ref(udp_response));
        if (action == "authenticate")
        {
            cout << "Server M has sent authentication request to Server A." << endl;
        }
        else if (action == "lookup")
        {
            cout << "The main server has sent the lookup request to server R." << endl;
        }
        else if (action == "push")
        {
            cout << "The main server has sent the push request to server R." << endl;
        }
        else if (action == "overwrite")
        {
            cout << "The main server has sent the overwrite confirmation response to server R." << endl;
        }
        else if (action == "deploy")
        {
            cout << "The main server has sent the deploy request to server D." << endl;
        }
        // Wait for the UDP response (ensure that the response has been obtained before responding to the TCP client)
        udp_thread.join();

        istringstream iss2(udp_response);
        string action2, param2;
        iss2 >> action2 >> param2;

        if (action2 == "overwrite")
        {
            cout << "The main server has received the response from server R using UDP over " << udp_port << ", asking for overwrite confirmation" << endl;
        }
        else if (action2 == "push")
        {
            cout << "The main server has received the response from server R using UDP over " << udp_port << endl;
        }
        else if (action2 == "remove")
        {
            if (param2 == "true")
            {
                cout << "The main server has received confirmation of the remove request done by the server R" << endl;
            }
        }
        else if (action2 == "auth")
        {
            cout << "The main server has received the response from server A using UDP over " << udp_port << endl;
        }

        if (action == "authenticate ")
        {
            cout << "The main server has received the response from server A using UDP over " << udp_port << endl;
        }
        else if (action == "lookup")
        {
            cout << "The main server has received the response from server R using UDP over " << udp_port << endl;
        }

        else if (action == "deploy")
        {
            cout << "The user " << param << "\'s repository has been deployed at server D." << endl;
        }

        // Send the UDP response back to the TCP client
        send(new_socket, udp_response.c_str(), udp_response.length(), 0);
        // cout << "Sent TCP response: " << udp_response << endl;
        if (action2 == "push")
        {
            cout << "The main server has sent the response to the client." << endl;
        }
        else if (action2 == "overwrite")
        {
            cout << "The main server has sent the overwrite confirmation request to the client." << endl;
        }

        if (action == "authenticate")
        {
            cout << "The main server has sent the response from server A to client using TCP over port " << tcp_port << endl;
        }
        else if (action == "lookup")
        {
            cout << "The main server has sent the response to the client." << endl;
        }

        close(new_socket); // Close the connection after responding
    }

    close(server_fd);
}

int main()
{
    int tcp_port = 25135;
    int udp_port = 24135;
    string udp_server_ip = "127.0.0.1"; // Example: UDP server address

    // Start TCP server to handle requests
    tcp_server(tcp_port, udp_server_ip, udp_port);

    return 0;
}
