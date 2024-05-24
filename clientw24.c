#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 4010
#define BUFFER_SIZE 1024

void handle_server_response(int sockfd) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t num_bytes = read(sockfd, buffer, BUFFER_SIZE - 1);

    if (num_bytes < 0) {
        perror("ERROR reading from socket");
        return;
    }

    buffer[num_bytes] = '\0';

    if (strncmp(buffer, "Filesize ", 9) == 0) {
        long filesize;
        sscanf(buffer + 9, "%ld", &filesize);
        printf("Expected filesize: %ld\n", filesize);

        mkdir("w24project", 0777);  // Ensure the directory exists

        FILE *fp = fopen("w24project/temp.tar.gz", "wb");
        if (!fp) {
            perror("Failed to open file");
            return;
        }

        long received = 0;
        while (received < filesize) {
            if (received == 0) { // If first read, buffer already contains part of the file
                char *fileDataStart = strchr(buffer, '\n') + 1;
                ssize_t initialDataSize = num_bytes - (fileDataStart - buffer);
                if (initialDataSize > 0) {
                    fwrite(fileDataStart, 1, initialDataSize, fp);
                    received += initialDataSize;
                }
            }

            memset(buffer, 0, BUFFER_SIZE);
            num_bytes = read(sockfd, buffer, (filesize - received < BUFFER_SIZE) ? filesize - received : BUFFER_SIZE);
            if (num_bytes > 0) {
                fwrite(buffer, 1, num_bytes, fp);
                received += num_bytes;
            } else if (num_bytes == 0) {
                break; // End of data
            } else {
                perror("Error reading from socket");
                break;
            }
        }
        fclose(fp);
        printf("Received file and saved to 'w24project/temp.tar.gz'\n");
    } else {
        printf("%s\n", buffer); // Just print the text message
    }
}

int verify_command_syntax(const char *command) {
    if (strncmp(command, "dirlist -a", strlen("dirlist -a")) == 0 ||
        strncmp(command, "dirlist -t", strlen("dirlist -t")) == 0 ||
        strncmp(command, "w24fn", strlen("w24fn")) == 0 ||
        strncmp(command, "w24fz", strlen("w24fz")) == 0 ||
        strncmp(command, "w24ft", strlen("w24ft")) == 0 ||
        strncmp(command, "w24fdb", strlen("w24fdb")) == 0 ||
        strncmp(command, "w24fda", strlen("w24fda")) == 0 ||
        strncmp(command, "quitc", strlen("quitc")) == 0) {
        return 1;
    } else {
        printf("Invalid command!\n");
        return 0;
    }
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Port>\n", argv[0]);
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        perror("ERROR on inet_pton");
        close(sockfd);
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on connect");
        close(sockfd);
        exit(1);
    }

    while (1) {
        printf("Enter command: ");
        memset(buffer, 0, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline

        if (!verify_command_syntax(buffer)) {
            continue; // Skip sending invalid commands
        }

        if (strcmp(buffer, "quitc") == 0) {
            if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
                perror("ERROR writing to socket");
            }
            break;
        }

        if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
            perror("ERROR writing to socket");
            break;
        }

        handle_server_response(sockfd);
    }

    close(sockfd);
    return 0;
}