
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_SIZE    256
#define MAX_CLIENTS 16

typedef struct {
    int socket;
    char name[MAX_SIZE];
} Client;

// global variables
Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

//sends message to all clients except the sender
void messageBroadcast(const char *message, int sender_socket)//
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) 
    {
        if (clients[i].socket != sender_socket) 
        {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

//takes a client struct as an argument and processes the client's messages 
void *process_client(void *arg)
{
    Client client = *(Client *)arg;
    char buffer[MAX_SIZE];
    char message[MAX_SIZE];

    while (1)
    {
        memset(buffer, 0, MAX_SIZE);
        int bytes_received = recv(client.socket, buffer, MAX_SIZE - 1, 0);
        if (bytes_received <= 0)
        {
            // Print the disconnection message only on the server terminal
            printf("%s disconnected\n", client.name);

            // Clean up the client's resources
            close(client.socket);
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < client_count; i++)
            {
                if (clients[i].socket == client.socket)
                {
                    clients[i] = clients[client_count - 1];
                    client_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            break;
        }

        // Null-terminate the received data
        buffer[bytes_received] = '\0';

        // Check if the message starts with '@' (whisper)
        if (strncmp(buffer, "@", 1) == 0)
        {
            char *target_name = strtok(buffer + 1, " ");
            char *msg = strtok(NULL, "\n");
            if (!target_name || !msg)
            {
                continue; // Malformed whisper message
            }

            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < client_count; i++)
            {
                if (strcmp(clients[i].name, target_name) == 0)
                {
                    if (snprintf(message, sizeof(message), "%s: %s\n", client.name, msg) >= (int)sizeof(message))
                    {
                        fprintf(stderr, "Error: Failed to format whisper for client %s\n", client.name);
                        message[0] = '\0';
                    }
                    send(clients[i].socket, message, strlen(message), 0);
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
        }
        else
        {
            // Broadcast the message to all other clients
            if (snprintf(message, sizeof(message), "%s: %s", client.name, buffer) >= (int)sizeof(message))
            {
                fprintf(stderr, "Error: Failed to format broadcast message from client %s\n", client.name);
                message[0] = '\0';
            }
            messageBroadcast(message, client.socket);
        }
    }

    return NULL;
}



void server_mode(int port)
{
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)//create a socket
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)//bind the socket to the address
    {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1)
    {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while (1)
    {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1)
        {
            perror("Accept failed");
            continue;
        }

        
        char ip_str[INET_ADDRSTRLEN];//IP converted to string
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);

        /* Receive the client-supplied name */
        char client_name[MAX_SIZE];
        memset(client_name, 0, MAX_SIZE);
        recv(client_socket, client_name, sizeof(client_name), 0);

        //print client connection
        printf("%s connected from %s\n", client_name, ip_str);

        pthread_mutex_lock(&clients_mutex);
        if (client_count < MAX_CLIENTS)
        {
            clients[client_count].socket = client_socket;
            strncpy(clients[client_count].name, client_name, sizeof(clients[client_count].name) - 1);
            clients[client_count].name[sizeof(clients[client_count].name) - 1] = '\0';
            client_count++;

            //client thread
            pthread_t thread;
            pthread_create(&thread, NULL, process_client, &clients[client_count - 1]);
            pthread_detach(thread);
        }
        else
        {
            const char *msg = "Server full\n";
            send(client_socket, msg, strlen(msg), 0);
            close(client_socket);
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    close(server_socket);
}

//client thread to receive messages from the server
void *get_message(void *arg)
{
    int client_socket = *(int *)arg;
    char buffer[MAX_SIZE];

    while (1)
    {
        memset(buffer, 0, MAX_SIZE);
        int bytes_received = recv(client_socket, buffer, MAX_SIZE - 1, 0);
        if (bytes_received <= 0)
        {
            /* Connection closed or error */
            printf("Server disconnected.\n");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    return NULL;
}


void client_mode(const char *addr, int port, const char *name)
{
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[MAX_SIZE];

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

   
    send(client_socket, name, strlen(name), 0);
    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, get_message, &client_socket);

    //stdin input loop
    while (1)
    {
        memset(buffer, 0, MAX_SIZE);

        if (!fgets(buffer, MAX_SIZE, stdin))//no input
        {
            break;
        }

        //handle !exit 
        if (strncmp(buffer, "!exit", 5) == 0)
        {
            send(client_socket, "!exit\n", strlen("!exit\n"), 0);
            printf("Client exiting.\n");
            break;
        }
        send(client_socket, buffer, strlen(buffer), 0);
    }
    close(client_socket);
}


int main(int argc, char *argv[]) 
{
    if (argc < 2)
    {
        exit(EXIT_FAILURE);
    }
    if (strcmp(argv[1], "hw3server") == 0) 
    {
        server_mode(atoi(argv[2]));
    } 
    else if (strcmp(argv[1], "hw3client") == 0) 
    {
        client_mode(argv[2], atoi(argv[3]), argv[4]);
    }
    return 0;
}
