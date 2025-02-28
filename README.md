# GitVerse

GitVerse is a distributed file repository system that allows users to manage and share files across multiple servers using TCP/UDP communication protocols.

## System Architecture

The system consists of five main components:

- **Main Server (serverM)**: Central coordinator that handles client requests and manages communication between other servers
- **Authentication Server (serverA)**: Handles user authentication
- **Repository Server (serverR)**: Manages file storage and retrieval
- **Deployment Server (serverD)**: Handles repository deployment
- **Client**: User interface for interacting with the system

## Features

- **User Authentication**: Supports both member and guest access
- **File Management**:
  - File lookup
  - File push/upload
  - File removal
  - Repository deployment
- **Command History**: Tracks user commands and provides logging functionality
- **Access Control**: Different permissions for members and guests

## Commands

### Member Commands

- `lookup <username>`: View files in a user's repository
- `push <filename>`: Upload a file to your repository
- `remove <filename>`: Remove a file from your repository
- `deploy`: Deploy your repository
- `log`: View your command history

### Guest Commands

- `lookup <username>`: View files in a user's repository (read-only access)

## Building and Running

### Prerequisites

- C++ compiler with C++11 support
- POSIX-compliant operating system

### Compilation

Use the provided Makefile to compile all components:

```bash
make
```

This will generate the following executables:

- serverA
- serverM
- serverR
- serverD
- client

### Running the System

1. Start all servers in separate terminals:

```bash
./serverA
./serverM
./serverR
./serverD
```

2. Run the client with username and password:

```bash
./client <username> <password>
```

For guest access:

```bash
./client guest guest
```

## Network Configuration

- Main Server (TCP): Port 25135
- Authentication Server (UDP): Port 21135
- Repository Server (UDP): Port 22135
- Deployment Server (UDP): Port 23135
- Response Port (UDP): Port 24135

## Security Features

- Password Encryption: Uses a cyclic shift cipher for password protection
- Access Control: Differentiates between member and guest privileges
- Authentication: Validates user credentials against stored member data

## File Structure

- `members.txt`: Stores user credentials
- `filenames.txt`: Maintains repository file records
- `deployed.txt`: Contains deployed repository information

## Notes

- All servers use localhost (127.0.0.1) for communication
- The system supports concurrent client connections
- File operations are atomic and thread-safe
