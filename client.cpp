#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdio>  // For perror
#include <cstdlib> // For exit()
#include <vector>
#include <fstream>
using namespace std;

#define SEND_PORT 25135
#define SERVER_IP "127.0.0.1"

// function to check whether a file exists
bool fileExists(const string &filename)
{
    ifstream file(filename);
    return file.good();
}

// Function to apply the encryption scheme
string encryptPassword(const string &password)
{
    string encrypted = password;

    for (char &c : encrypted)
    {
        if (isalpha(c))
        {
            // Handle alphabetic characters (cyclic shift with case sensitivity)
            if (islower(c))
            {
                c = 'a' + (c - 'a' + 3) % 26;
            }
            else if (isupper(c))
            {
                c = 'A' + (c - 'A' + 3) % 26;
            }
        }
        else if (isdigit(c))
        {
            // Handle numeric digits (cyclic shift)
            c = '0' + (c - '0' + 3) % 10;
        }
        // Special characters and spaces remain unchanged
    }

    return encrypted;
}

void send_tcp_request(const string &message)
{
    istringstream isst(message);
    string command, param1, param2;
    isst >> command >> param1 >> param2;

    int sock;
    struct sockaddr_in server_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return;
    }

    // Set up the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SEND_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address/Address not supported");
        close(sock);
        return;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(sock);
        return;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if (getsockname(sock, (struct sockaddr *)&client_addr, &client_addr_len) == -1)
    {
        perror("getsockname() failed");
        close(sock);
        return;
    }

    // Send the message
    send(sock, message.c_str(), message.size(), 0);
    // cout << "Request sent to the server: " << message << endl;

    // Receive the server's response
    char buffer[1024] = {0};
    int bytes_read = read(sock, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0)
    {
        // cout << "Server response: " << buffer << endl;
    }
    if (command == "log")
    {
        cout << "The client received the response from the main server using TCP over port " << ntohs(client_addr.sin_port) << endl;
        cout << string(buffer) << endl;
        cout << "----Start a new request----" << endl;
        close(sock);
        return;
    }

    istringstream iss(buffer);
    string action, response;
    iss >> action >> response;
    if (action == "auth")
    {
        if (response == "false")
        {
            cout << "The credentials are incorrect. Please try again." << endl;
            exit(1);
        }
        else if (response == "true")
        {
            cout << "You have been granted member access" << endl;
        }
    }
    else if (action == "lookup")
    {
        int count = stoi(response);
        // cout << "The user has  " << count << " files in their repository." << endl;
        cout << "The client received the response from the main server using TCP over port " << ntohs(client_addr.sin_port) << endl;
        if (count == 0)
        {
            cout << "Empty repository." << endl
                 << "----Start a new request----" << endl;
        }
        else if (count == -1)
        {
            cout << param1 << " does not exist. Please try again." << endl
                 << "----Start a new request----" << endl;
        }
        else
        {
            std::vector<std::string> filenames;
            std::string filename;

            while (iss >> filename)
            {
                filenames.push_back(filename);
            }

            for (const auto &filename : filenames)
            {
                cout << filename << endl;
            }
            cout << "----Start a new request----" << endl;
        }
    }
    else if (action == "overwrite")
    {
        string answer = "";
        cout << param1 << " exists in " << param2 << "\'s repository, do you want to overwrite (Y/N)? " << endl;
        cin >> answer;
        if (answer == "Y" || answer == "y")
        {
            send_tcp_request("overwrite true " + param1 + " " + param2);
        }
        else if (answer == "N" || answer == "n")
        {
            send_tcp_request("overwrite false " + param1 + " " + param2);
        }
    }
    else if (action == "push")
    {
        if (response == "true")
        {
            cout << param2 << " pushed successfully." << endl;
        }
        else if (response == "false")
        {
            cout << param2 << " was not pushed successfully." << endl;
        }
    }
    else if (action == "remove")
    {
        if (response == "true")
        {
            cout << "The remove request was successful." << endl;
        }
    }

    else if (action == "deploy")
    {
        string username, filename;
        username = response;
        iss >> filename;

        cout << "The client received the response from the main server using TCP over port " << ntohs(client_addr.sin_port) << ". The following files in his/her repository have been deployed." << endl;
        cout << filename << endl;
        while (iss >> username >> filename)
        {
            cout << filename << endl;
        }
        cout << "----Start a new request----" << endl;
    }

    // Close the socket
    close(sock);
}

int main(int argc, char *argv[])
{
    cout << "The client is up and running." << endl;
    // Check if the number of arguments is correct
    if (argc != 3)
    {
        cout << "The credentials are incorrect. Please try again." << endl;
        return 1;
    }

    // Get the username and password from the command line arguments
    string username = argv[1];
    string password = argv[2];

    // Print the username and password
    // cout << "Username: " << username << endl;
    // cout << "Password: " << password << endl;

    bool isMember = true;
    if (username == "guest" && password == "guest")
    {
        isMember = false;
        cout << "You have been granted guest access" << endl;
    }

    string command;
    if (isMember)
    {
        // cout << "authenticate " << username << " " << encryptPassword(password) << endl;

        // check if the user is a member
        send_tcp_request("authenticate " + username + " " + encryptPassword(password));

        cout << "Please enter the command: <lookup <username>>, <push <filename>>, <remove <filename>>, <deploy>, <log>." << endl;
        while (true)
        {
            cout << "> ";
            getline(cin, command);

            // Parse the command
            istringstream iss(command);
            string action, param;
            iss >> action >> param;

            // case 1: lookup <username>
            if (action == "lookup")
            {
                if (param.empty())
                {
                    // If the username is not specified, lookup the member's username
                    // the member's username is the username that the client entered when logging in
                    cout << "Username is not specified. Will lookup " << username << endl;
                    cout << username << " sent a lookup request to the main server." << endl;
                    send_tcp_request("lookup " + username + " " + username);
                }
                else
                {
                    // If the username is specified, lookup the username
                    cout << username << " sent a lookup request to the main server." << endl;
                    send_tcp_request("lookup " + param + " " + username);
                }
            }

            // case 2: push <filename>
            else if (action == "push")
            {
                // condition: no filename is specified
                if (param.empty())
                {
                    cout << "Error: Filename is required. Please specify a filename to push." << endl;
                }
                else
                {
                    if (!fileExists(param))
                    {
                        cout << "Error: Invalid file: " << param << endl
                             << "----Start a new request----" << endl;
                        continue;
                    }
                    send_tcp_request("push " + param + " " + username);
                }
            }

            // case 3: deploy
            else if (action == "deploy")
            {
                // condition: Upon sending a deploy request
                cout << username << " sent a deploy request to the main server." << endl;
                send_tcp_request("deploy " + username);
            }

            // case 4: remove <filename>
            else if (action == "remove")
            {
                // condition: Upon sending a remove request
                cout << username << " sent a remove request to the main server" << endl;
                send_tcp_request("remove " + param + " " + username);
            }

            // case 5: log
            else if (action == "log")
            {
                // condition: Upon sending a log request
                cout << username << " sent a log request to the main server" << endl;
                send_tcp_request("log " + username);
            }

            else
            {
                cout << "Please enter the command: <lookup <username>>, <push <filename>>, <remove <filename>>, <deploy>, <log>." << endl;
            }
        }
    }
    else
    {
        cout << "Please enter the command: <lookup <username>>." << endl;
        while (true)
        {
            cout << "> ";
            getline(cin, command);

            // Parse the command
            istringstream iss(command);
            string action, param;
            iss >> action >> param;

            // case 1: lookup <username>
            if (action == "lookup")
            {
                if (param.empty())
                {
                    // If the username is not specified, lookup the member's username
                    // the member's username is the username that the client entered when logging in
                    cout << "Error: Username is required. Please specify a username to lookup. " << endl
                         << "----Start a new request----" << endl;
                }
                else
                {
                    // If the username is specified, lookup the username
                    cout << "Guest sent a lookup request to the main server." << endl;
                    send_tcp_request("lookup " + param + " " + username);
                }
            }
            else
            {
                cout << "Guests can only use the lookup command." << endl;
            }
        }
    }
}