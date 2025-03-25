
# 📡 TCP Chat Application

## 📄 Overview

This project implements a **multi-client TCP-based chat system** in **C** for **Linux environments**. It consists of a **server** component that handles multiple client connections simultaneously and a **client** component allowing users to connect to the server and chat with other clients.

## ✨ Features

- **Multi-client Support**: Handles up to **16 concurrent client connections**.
- **Broadcast Messaging**: Messages from clients are broadcast to all connected users.
- **Private Messaging**: Supports direct messaging using the `@username` syntax.
- **Clean Disconnection**: Proper handling of client exits.
- **Thread Safety**: Uses **mutex locks** to protect shared resources.
- **User Identification**: Each client has a **unique username**.

## 🔧 Components

### Server

The server listens for incoming client connections on a specified port. For each new connection, the server:

1. Accepts the connection and receives the client's username.
2. Adds the client to the active clients list.
3. Creates a dedicated thread to handle the client's messages.
4. Broadcasts messages from one client to all others.
5. Facilitates **private messaging** between clients.

### Client

The client connects to the server using the server's IP address and port number. It can:

1. Connect to the server and register a username.
2. Listen for and display incoming messages from the server.
3. Read user input from the console and send messages.
4. Support special commands like `!exit` to disconnect.
5. Support private messaging via `@username`.

## 🚀 Usage

### Server Mode

To start the server:

```bash
./hw3 hw3server <port>
```

Example:

```bash
./hw3 hw3server 8080
```

### Client Mode

To start a client:

```bash
./hw3 hw3client <server_ip> <port> <username>
```

Example:

```bash
./hw3 hw3client 127.0.0.1 8080 Alice
```

## 💬 Message Types

### Broadcast Message

Send a message to all connected clients:

```
Hello everyone!
```

### Private Message (Whisper)

Send a private message to a specific user:

```
@Bob Hey, how are you?
```

### Exit Command

Disconnect from the server:

```
!exit
```

## 🔍 Technical Implementation

### Data Structures

- **Client struct**: Stores the client socket descriptor and username.
- **Array of Client structures**: Tracks all connected clients.

### Thread Management

- Each client is handled by a **dedicated thread**.
- **`pthread_mutex_t`** ensures thread-safe access to shared resources.
- Threads are **detached** to clean up resources automatically.

### Networking

- Uses **TCP sockets** for reliable communication.
- Proper handling of connection, transmission, and disconnection.
- Processes **partial messages** and **connection failures**.

### Synchronization

- **Mutex locks** protect shared data.
- Proper error handling for **socket operations** and **thread creation**.


