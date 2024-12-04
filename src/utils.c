#include "two_threads.h"
#include "tries.h"
#include "lru.h"
#include "log.h"
// #include <pthread.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <pthread.h>
// #include <errno.h>

// #pragma pack(push, 1) // Prevent padding
// typedef struct
// {
//     int request_type;
//     char data[1024];
// } Packet;
// #pragma pack(pop)

extern StorageServerInfo storage_servers[MAX_STORAGE_SERVERS];
extern int storage_server_sockets[MAX_STORAGE_SERVERS];
extern int storage_server_count;
extern pthread_mutex_t lock;
extern TrieNode *trieRoot;

int send_message_to_host(char *ip, int port, Packet *packet)
{
    int sockfd;
    struct sockaddr_in server_addr;
    int status = 0;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed at send_message_to_host function");
        return -1;
    }

    // Configure server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IP address from text to binary
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid IP address");
        close(sockfd);
        return -1;
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }

    printf("Connected to %s:%d\n", ip, port);

    // Send the message
    if (send(sockfd, packet, sizeof(Packet), 0) < 0)
    {
        perror("Message send failed at send_message_to_host function");
        status = -1;
    }
    else
    {
        printf("Message sent at send_message_to_host function\n");
    }

    // Close the socket
    close(sockfd);

    return status;
}
// void main()
// {
//     Packet send;
//     snprintf(&send.data, sizeof(Packet), "message");
//     send.request_type = 9;
//     send_message_to_host("10.42.0.25", 8083, &send);
// }
int status_check(char *ip, int port)
{
    for (int i = 0; i < storage_server_count; i++)
    {
        if (strcmp(storage_servers[i].ip, ip) == 0 && port == storage_servers[i].client_port)
        {
            return storage_servers[i].status;
        }
    }
    return 0;
}

char *extract_file_name(char *path)
{
    char *file_name = strrchr(path, '/'); // Look for the last '/' in the path
    if (file_name)
    {
        return file_name + 1; // Skip the '/' to get the file name
    }
    return path; // If no '/' is found, the entire path is the file name
}

// Function to find a storage server by IP address
int find_storage_server(const char *address, const int clientPort)
{
    for (int i = 0; i < storage_server_count; i++)
    {
        if (strcmp(storage_servers[i].ip, address) == 0 && clientPort == storage_servers[i].client_port)
        {
            return i; // Found
        }
    }
    return -1; // Not found
}

// Helper function to check if a path exists in the server's paths
int path_exists(StorageServerInfo *server, const char *path)
{
    for (int i = 0; i < server->pathscount; i++)
    {
        if (strcmp(server->paths[i], path) == 0)
        {
            return 1; // Path exists
        }
    }
    return 0; // Path does not exist
}
void add_path(StorageServerInfo *storageServer, char *directory, char *fileName)
{
    for (int i = 0; i < storageServer->pathscount; i++)
    {
        if (strcmp(directory, storageServer->paths[i]) == 0)
        {
            printf("Iterated to %s at index %d\n", storageServer->paths[i], i);

            // Not checking if the files are less than 100
            for (int j = storageServer->pathscount - 1; j > i; j--)
            {
                printf("Moving %d to %d\n", j, j + 1);
                strcpy(storageServer->paths[j + 1], storageServer->paths[j]);
            }
            // char fullpath[MAX_PATH_LENGTH];
            printf("Adding full path %s/%s at index %d\n", directory, fileName, i + 1);
            snprintf(storageServer->paths[i + 1], MAX_PATH_LENGTH, "%s/%s", directory, fileName);
            insertPath(trieRoot, storageServer->paths[i + 1], storageServer->ip, storageServer->client_port);
            storageServer->pathscount++;
        }
    }
}
// Function to get the path after a specific folder name
const char *get_path_after_folder(const char *full_path, const char *folder_name)
{
    const char *folder_position = strstr(full_path, folder_name);
    if (folder_position)
    {
        // Move the pointer to the end of the folder name
        folder_position += strlen(folder_name);
        // Ensure it starts with a '/' if the path continues
        if (*folder_position == '/')
            return folder_position + 1; // Skip the '/'
    }
    return NULL; // Folder name not found in the path
}
char *remove_prefix(char *str)
{
    // Check if the string starts with "./"
    if (strstr(str, "./") == str)
    {
        return str + 2; // Skip the first two characters
    }
    return str; // Return the original string if no "./" prefix
}
void add_paths(StorageServerInfo *source, StorageServerInfo *dest, char *srcPath, char *destPath)
{
    for (int i = 0; i < source->pathscount; i++)
    {
        if (strncmp(source->paths[i], srcPath, strlen(srcPath)) == 0)
        {
            char *present_dir = NULL;
            const char *slash_position = strrchr(srcPath, '/'); // Find the last '/'
            if (slash_position)
            {
                present_dir = (char *)(slash_position + 1); // Valid pointer to the part after the '/'
            }
            else
            {
                present_dir = ""; // If no '/' is found, set to an empty string
            }
            printf("Present_dir: %s\n", present_dir);
            char path[MAX_PATH_LENGTH];
            char *relative_path = get_path_after_folder(source->paths[i], srcPath);
            printf("relatie_path:%s\n", relative_path);
            if (relative_path == NULL)
            {
                relative_path = "\0";
                snprintf(path, MAX_PATH_LENGTH, "%s/%s", destPath, present_dir);
            }
            else
            {
                snprintf(path, MAX_PATH_LENGTH, "%s/%s/%s", destPath, present_dir, relative_path);
            }
            snprintf(dest->paths[dest->pathscount], MAX_PATH_LENGTH, path);
            insertPath(trieRoot, dest->paths[dest->pathscount], dest->ip, dest->client_port);
            printf("Path Added:%s\n", path);
            dest->pathscount++;
        }
    }
}

int compare_2d_arrays(const char arr1[][MAX_PATH_LENGTH], const char arr2[][MAX_PATH_LENGTH], size_t row_count)
{
    for (size_t i = 0; i < row_count; i++)
    {
        if (strcmp(arr1[i], arr2[i]) != 0)
        {
            return 0; // Not equal
        }
    }
    return 1; // Equal
}

void clear_socket_buffer(int socket_fd)
{
    char temp_buffer[BUFFER_LENGTH];
    ssize_t bytes_read;
    do
    {
        bytes_read = recv(socket_fd, temp_buffer, sizeof(temp_buffer), MSG_DONTWAIT);
    } while (bytes_read > 0);
}

void modify_backup()
{
    // pthread_mutex_lock(&lock);
    for (int i = 0; i < storage_server_count; i++)
    {
        ClientMessage msg;
        memset(&msg, 0, sizeof(ClientMessage));
        msg.request_type = DELETE_REQ;
        snprintf(msg.path, MAX_PATH_LENGTH, "%s/backup", storage_servers[i].paths[0]);
        printf("Client Request: Request_type:%d path:%s name:%s content:%s destination_path:%s\n",
               msg.request_type, msg.path, msg.name, msg.content, msg.dest_path);
        // send(storage_server_sockets[i], &msg, sizeof(ClientMessage), 0);
        handle_delete_request(&msg, -1);
        memset(&msg, 0, sizeof(ClientMessage));
        msg.request_type = CREATE_REQ;
        snprintf(msg.path, MAX_PATH_LENGTH, "%s", storage_servers[i].paths[0]);
        snprintf(msg.name, FILENAME_LENGTH, "backup");
        printf("Client Request: Request_type:%d path:%s name:%s content:%s destination_path:%s\n",
               msg.request_type, msg.path, msg.name, msg.content, msg.dest_path);
        // send(storage_server_sockets[i], &msg, sizeof(ClientMessage), 0);
        handle_create_request(-1, &msg);
    }
    for (int i = 0; i < storage_server_count; i++)
    {
        for (int j = i + 1; j <= i + 1; j++)
        {
            ClientMessage msg;
            memset(&msg, 0, sizeof(ClientMessage));
            msg.request_type = COPY_REQ;
            snprintf(msg.path, MAX_PATH_LENGTH, "%s", storage_servers[i].paths[0]);
            snprintf(msg.dest_path, MAX_PATH_LENGTH, "%s/backup", storage_servers[(j % storage_server_count)].paths[0]);
            printf("Client Request: Request_type:%d path:%s name:%s content:%s destination_path:%s\n",
                   msg.request_type, msg.path, msg.name, msg.content, msg.dest_path);
            copy(&msg, -1);
            memset(&msg, 0, sizeof(ClientMessage));
            msg.request_type = DELETE_REQ;
            snprintf(msg.path, MAX_PATH_LENGTH, "%s/backup/%s/backup", storage_servers[(j % storage_server_count)].paths[0], remove_prefix(storage_servers[i].paths[0]));
            printf("Client Request: Request_type:%d path:%s name:%s content:%s destination_path:%s\n",
                   msg.request_type, msg.path, msg.name, msg.content, msg.dest_path);
            handle_delete_request(&msg, -1);
        }
    }

    // pthread_mutex_unlock(&lock);
}
