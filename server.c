#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 2
#define FILE_NAME_SIZE 100

// Function to run the Python script
void run_python_script(const char *filename) {
    char command[256];
    snprintf(command, sizeof(command), "python3 run.py %s", filename);
    system(command);
}


void send_file(int sockfd, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("File open error");
        exit(EXIT_FAILURE);
    }

    // Get file size
    fseek(fp, 0L, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    // Send file size first
    send(sockfd, &file_size, sizeof(file_size), 0);
    printf("Sent file size: %zu bytes\n", file_size);

    // Send the actual file data
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        send(sockfd, buffer, bytes_read, 0);
    }

    fclose(fp);
    printf("File %s sent to server\n", filename);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char filename[FILE_NAME_SIZE] = "received.wav";

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept client connections and process them in a loop
    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected...\n");

        // Receive the file size first
        size_t file_size = 0;
        if (recv(client_fd, &file_size, sizeof(file_size), 0) <= 0) {
            perror("Failed to receive file size");
            close(client_fd);
            continue;
        }
        printf("Receiving file of size: %zu bytes\n", file_size);

        // Receive the .wav file from the client
        FILE *fp = fopen(filename, "wb");
        if (fp == NULL) {
            perror("Failed to open file");
            close(client_fd);
            continue;
        }

        ssize_t bytes_received;
        size_t total_received = 0;

        while (total_received < file_size) {
            bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                break;  // Either connection closed or error occurred
            }
            fwrite(buffer, sizeof(char), bytes_received, fp);
            total_received += bytes_received;
        }

        fclose(fp);
        printf("Received and saved the file: %s\n", filename);

        // Run the Python script to generate 10 .wav files
        printf("Running Python script on received file...\n");
        run_python_script(filename);

        // Send the 10 generated .wav files to the client
        for (int i = 0; i < 10; i++) {
            char output_filename[FILE_NAME_SIZE];
            snprintf(output_filename, FILE_NAME_SIZE, "output%d.wav", i+1);

            send_file(client_fd, output_filename);
        }

        printf("All files sent to the client.\n");

        // Close the client connection and wait for another one
        close(client_fd);
        printf("Client disconnected. Waiting for new connection...\n");
    }

    // Close the server socket
    close(server_fd);

    return 0;
}
