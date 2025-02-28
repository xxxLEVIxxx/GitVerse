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
#define RECEIVE_PORT 22135
#define RESPONSE_PORT 24135

string getEntries(const string &username, const string &filepath)
{
    ifstream file(filepath); // Open the file for reading
    if (!file.is_open())
    {
        cerr << "Error: Could not open the file." << endl;
        return "";
    }

    string line;
    string result; // String to store all entries belonging to the username

    // Read the file line by line
    while (getline(file, line))
    {
        istringstream ss(line);
        string file_username, file_name;

        // Extract the username and filename from the line
        ss >> file_username >> file_name;

        // If the username matches, add the line to the result
        if (file_username == username)
        {
            result += line + " ";
        }
    }

    file.close();
    return result;
}

void removeEntry(const string &username, const string &filename, const string &filepath)
{
    ifstream file(filepath); // Open the file for reading
    if (!file.is_open())
    {
        cerr << "Error: Could not open the file for reading." << endl;
        return;
    }

    ofstream tempFile("temp.txt"); // Open a temporary file for writing
    if (!tempFile.is_open())
    {
        cerr << "Error: Could not open the temporary file for writing." << endl;
        file.close();
        return;
    }

    string line;
    bool removed = false;

    // Read the original file line by line
    while (getline(file, line))
    {
        istringstream ss(line);
        string file_username, file_name;

        // Extract the username and filename from the line
        ss >> file_username >> file_name;

        // If the line does not match the username and filename, write it to the temporary file
        if (!(file_username == username && file_name == filename))
        {
            tempFile << line << endl;
        }
        else
        {
            removed = true; // Mark that the entry was found and removed
        }
    }

    file.close();
    tempFile.close();

    // Replace the original file with the temporary file
    if (removed)
    {
        if (rename("temp.txt", filepath.c_str()) != 0)
        {
            cerr << "Error: Could not update the file." << endl;
        }
        else
        {
            // cout << "The entry has been successfully removed." << endl;
        }
    }
    else
    {
        // Remove the temporary file if no match was found
        // cout << "No matching entry found." << endl;
        remove("temp.txt");
    }
}

void pushToRepo(const string &username, const string &filename, const string &filepath)
{
    ofstream file(filepath.c_str(), ios::app);
    file << username << " " << filename << endl;
    file.close();
}

bool isExist(const string &username, const string &filename, const string &filepath)
{
    ifstream file(filepath.c_str());
    string line;

    if (!file.is_open())
    {
        cerr << "Error: Could not open the file." << endl;
        return false; // Return "0" if the file cannot be opened
    }

    // Read the file line by line
    while (getline(file, line))
    {
        istringstream ss(line);
        string file_username, file_name;

        // Extract the username and password from the line
        ss >> file_username >> file_name;

        // Check if the username matches the given one
        if (file_username == username && file_name == filename)
        {
            return true; // Return "exist true" if the username exists
        }
    }

    file.close();
    return false; // Return "exist false" if the username does not exist
}

string lookup(const string &username, const string &filename)
{
    ifstream file(filename);
    string line;
    vector<string> files;

    if (!file.is_open())
    {
        cerr << "Error: Could not open the file." << endl;
        return "0"; // Return "0" if the file cannot be opened
    }

    // Read the file line by line
    while (getline(file, line))
    {
        istringstream ss(line);
        string file_username, file_name;

        // Extract the username and filename from the line
        ss >> file_username >> file_name;

        // Check if the username matches the given one
        if (file_username == username)
        {
            files.push_back(file_name); // Add the filename to the list
        }
    }

    file.close();

    // Build the result string
    string result = to_string(files.size()); // Start with the total number of files
    for (const auto &file_name : files)
    {
        result += " " + file_name; // Append each filename with a space
    }

    return result;
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
    std::cout << "Server R is up and running using UDP on port " << RECEIVE_PORT << "..." << std::endl;

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
        string action, param, username;
        iss >> action >> param >> username;
        // std::cout << "ServerA received username " << username << " and " << password << std::endl;
        // std::cout << "ServerA received username " << param << " and ****** " << std::endl;
        std::string response;
        if (action == "lookup")
        {
            cout << "Server R has received lookup request from the main server." << endl;
            response = "lookup " + lookup(param, "./filenames.txt");
        }
        else if (action == "push")
        {
            cout << "Server R has received push request from the main server." << endl;
            if (isExist(username, param, "./filenames.txt"))
            {
                cout << param << " exists in " << username << "\'s repository; requesting overwrite confirmation" << endl;
                response = "overwrite";
            }
            else
            {
                pushToRepo(username, param, "./filenames.txt");
                cout << param << " uploaded successfully." << endl;
                response = "push true";
            }
        }
        else if (action == "overwrite")
        {
            if (param == "true")
            {
                cout << "User requested overwrite; overwrite successful." << endl;
                response = "push true";
            }
            else
            {
                cout << "Overwrite denied" << endl;
                response = "push false";
            }
        }
        else if (action == "remove")
        {
            cout << "Server R has received a remove request from the main server." << endl;
            removeEntry(username, param, "./filenames.txt");
            response = "remove true";
        }
        else if (action == "deploy")
        {
            cout << "Server R has received a deploy request from the main server." << endl;
            string result = getEntries(param, "./filenames.txt");
            cout << result << endl;
            response = "deploy " + result;
        }
        else
        {
            cout << "Acknowledged: " << buffer << endl;
            response = "Acknowledged: " + std::string(buffer);
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
        if (action == "lookup")
        {
            cout << "Server R has finished sending the response to the main server." << endl;
        }
        else if (action == "deploy")
        {
            cout << "Server R has finished sending the response to the main server." << endl;
        }
    }

    close(server_fd);
}

int main()
{
    start_udp_server();
    return 0;
}