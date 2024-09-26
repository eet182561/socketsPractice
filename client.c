#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"  // Change to the server's IP address
#define PORT 8080
#define BUFFER_SIZE 2
#define FILE_NAME_SIZE 100

// Function to record audio using ffmpeg
void record_audio(const char *filename) {
    printf("Recording... Press Enter to stop\n");
    system("ffmpeg -f avfoundation -i :0 -t 0:30 -y temp.wav"); // 30s recording

    // Convert the recorded audio to a proper .wav file
    char command[256];
    snprintf(command, sizeof(command), "ffmpeg -i temp.wav -acodec pcm_s16le -ar 16000 -ac 2 %s -y", filename);
    system(command);

    printf("Recording saved as %s\n", filename);
}

// Function to send a file to the server
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

void recieve_file(int sockfd, const char *filename){
    char buffer[BUFFER_SIZE];
    size_t file_size = 0;
    if (recv(sockfd, &file_size, sizeof(file_size), 0) <= 0) {
        perror("Failed to receive file size");
        close(sockfd);
        return;
    }
    printf("File Size for %s: %ld\n",filename, file_size);
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("Failed to open file");
        close(sockfd);
        return;
    }
    ssize_t bytes_received;
    size_t total_received = 0;

    while (total_received < file_size) {
        bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;  // Either connection closed or error occurred
        }
        fwrite(buffer, sizeof(char), bytes_received, fp);
        total_received += bytes_received;
    }
    fclose(fp);
    printf("Received and saved the file: %s\n", filename);
}

// Function to receive files from the server
void receive_files(int sockfd) {
    char buffer[BUFFER_SIZE];
    for (int i = 0; i < 10; i++) {
        char output_filename[FILE_NAME_SIZE];
        snprintf(output_filename, FILE_NAME_SIZE, "received_output%d.wav", i + 1);
        recieve_file(sockfd, output_filename);
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char filename[FILE_NAME_SIZE] = "client_audio.wav";

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server.\n");

    // Main loop for sending and receiving audio
    while (1) {
        printf("Press Enter to start recording...\n");
        getchar();  // Wait for the user to press Enter

        // Record audio
        record_audio(filename);

        // Send the recorded .wav file to the server
        send_file(sockfd, filename);

        // Receive the 10 generated .wav files from the server
        receive_files(sockfd);

        printf("Press Enter to record and send another file, or Ctrl+C to exit.\n");
        getchar();  // Wait for the user to press Enter again to start the next cycle
    }

    // Close the socket
    close(sockfd);

    return 0;
}
