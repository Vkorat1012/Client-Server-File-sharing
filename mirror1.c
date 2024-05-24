
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #include <strings.h> // For strcasecmp
    #include <time.h>
    #include <fcntl.h> // For file control options
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>

    #define _XOPEN_SOURCE 700  // or you can use #define _XOPEN_SOURCE
    #define PORT 4011
    #define BUFFER_SIZE 1024


    typedef struct {
        char *name;
        time_t creation_time; // Creation time
    } DirEntry;

    void handle_find_before_date_request(int client_fd, char *date) {
        char find_command[1024];
        char temp_file_list[] = "temp_file_list_by_date.txt";
        struct tm tm = {0};  // Structure to hold time components
        time_t date_time;

        // Parse date input and convert to time_t
        if (strptime(date, "%Y-%m-%d", &tm) == NULL) {
            write(client_fd, "Invalid date format\n", 20);
            return;
        }
        tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0; tm.tm_isdst = -1;
        date_time = mktime(&tm);
        if (date_time == -1) {
            write(client_fd, "Invalid date\n", 13);
            return;
        }

        // Format the find command to fetch files older than the specified date
        snprintf(find_command, sizeof(find_command),
            "find ~ -type f -newermt '1970-01-01' ! -newermt '%s' \\( ! -regex '.*/\\..*' \\) > %s",
            date, temp_file_list);

        // Execute the find command
        system(find_command);

        // Check if the file list is empty
        struct stat statbuf;
        if (stat(temp_file_list, &statbuf) != 0 || statbuf.st_size == 0) {
            write(client_fd, "No file found\n", 14);
            remove(temp_file_list);
            return;
        }

        // Create a tar file from the list of files found
        char tar_command[1024];
        snprintf(tar_command, sizeof(tar_command), "tar -czf temp.tar.gz -T %s", temp_file_list);
        system(tar_command);

        // Check if the tar was successful
        int tar_fd = open("temp.tar.gz", O_RDONLY);
        if (tar_fd < 0) {
            write(client_fd, "Error creating tar file\n", 24);
            remove(temp_file_list);
            return;
        }

        if (fstat(tar_fd, &statbuf) != 0 || statbuf.st_size == 0) {
            write(client_fd, "No file found\n", 14);
            close(tar_fd);
            remove(temp_file_list);
            return;
        }

        // Send the tarball file size
        char file_size_msg[64];
        snprintf(file_size_msg, sizeof(file_size_msg), "Filesize %ld\n", statbuf.st_size);
        write(client_fd, file_size_msg, strlen(file_size_msg));

        // Send the tarball file content
        char file_buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(tar_fd, file_buffer, sizeof(file_buffer))) > 0) {
            write(client_fd, file_buffer, bytes_read);
        }

        close(tar_fd);
        remove("temp.tar.gz");
        remove(temp_file_list);
    }

    void handle_find_after_date_request(int client_fd, char *date) {
        char find_command[1024];
        char temp_file_list[] = "temp_file_list_after_date.txt";
        struct tm tm = {0};  // Structure to hold time components
        time_t date_time;

        // Parse date input and convert to time_t
        if (strptime(date, "%Y-%m-%d", &tm) == NULL) {
            write(client_fd, "Invalid date format\n", 20);
            return;
        }
        tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0; tm.tm_isdst = -1;
        date_time = mktime(&tm);
        if (date_time == -1) {
            write(client_fd, "Invalid date\n", 13);
            return;
        }

        // Format the find command to fetch files created on or after the specified date
        snprintf(find_command, sizeof(find_command),
            "find ~ -type f -newermt '%s' \\( ! -regex '.*/\\..*' \\) > %s",
            date, temp_file_list);

        // Execute the find command
        system(find_command);

        // Check if the file list is empty
        struct stat statbuf;
        if (stat(temp_file_list, &statbuf) != 0 || statbuf.st_size == 0) {
            write(client_fd, "No file found\n", 14);
            remove(temp_file_list);
            return;
        }

        // Create a tar file from the list of files found
        char tar_command[1024];
        snprintf(tar_command, sizeof(tar_command), "tar -czf temp.tar.gz -T %s", temp_file_list);
        system(tar_command);

        // Check if the tar was successful
        int tar_fd = open("temp.tar.gz", O_RDONLY);
        if (tar_fd < 0) {
            write(client_fd, "Error creating tar file\n", 24);
            remove(temp_file_list);
            return;
        }

        if (fstat(tar_fd, &statbuf) != 0 || statbuf.st_size == 0) {
            write(client_fd, "No file found\n", 14);
            close(tar_fd);
            remove(temp_file_list);
            return;
        }

        // Send the tarball file size
        char file_size_msg[64];
        snprintf(file_size_msg, sizeof(file_size_msg), "Filesize %ld\n", statbuf.st_size);
        write(client_fd, file_size_msg, strlen(file_size_msg));

        // Send the tarball file content
        char file_buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(tar_fd, file_buffer, sizeof(file_buffer))) > 0) {
            write(client_fd, file_buffer, bytes_read);
        }

        close(tar_fd);
        remove("temp.tar.gz");
        remove(temp_file_list);

    }

    void handle_file_type_request(int client_fd, char *extensions) {
        char *token;
        char find_command[1024] = "find ~ -type f \\(";
        char temp_file_list[] = "temp_file_list.txt";   //Temporary file to store results
        int count = 0;  // Counter for file extensions
        int first = 1; //Flag to handle the first token specially

        // Parse the extensions and prepare the find command
        token = strtok(extensions, " ");
        while (token != NULL) {
            if (count >= 3) {  // Check if more than three extensions are provided
                write(client_fd, "Only up to 3 file types can be specified\n", 40);
                return;
            }
            if (!first) {
                strcat(find_command, " -o");
            }
            strcat(find_command, " -name '*.");     // Append each file extension to the find command
            strcat(find_command, token);
            strcat(find_command, "'");
            first = 0;
            token = strtok(NULL, " ");
            count++;  // Increment the count for each file extension
        }
        strcat(find_command, " \\) ! -regex '.*/\\..*' > ");
        strcat(find_command, temp_file_list);

        // If no valid extensions were given
        if (count == 0) {
            write(client_fd, "At least one file type must be specified\n", 41);
            return;
        }

        // Execute the find command and store results in temp_file_list
        system(find_command);

        // Check if the file list is empty
        struct stat statbuf;
        if (stat(temp_file_list, &statbuf) != 0 || statbuf.st_size == 0) {
            write(client_fd, "No file found\n", 14);
            return;
        }

        // Create a tar file from the list of files found
        char tar_command[1024];
        snprintf(tar_command, sizeof(tar_command), "tar -czf temp.tar.gz -T %s 2>/dev/null", temp_file_list);

        system(tar_command);

        // Check if the tar was successful
        int tar_fd = open("temp.tar.gz", O_RDONLY);
        if (tar_fd < 0) {
            write(client_fd, "Error creating tar file\n", 24);
            remove(temp_file_list);
            return;
        }

        if (fstat(tar_fd, &statbuf) != 0 || statbuf.st_size == 0) {
            write(client_fd, "No file found\n", 14);
            close(tar_fd);
            remove(temp_file_list);
            return;
        }

        // Send the tarball file size
        char file_size_msg[64];
        snprintf(file_size_msg, sizeof(file_size_msg), "Filesize %ld\n", statbuf.st_size);
        write(client_fd, file_size_msg, strlen(file_size_msg));

        // Send the tarball file content
        char file_buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(tar_fd, file_buffer, sizeof(file_buffer))) > 0) {
            write(client_fd, file_buffer, bytes_read);
        }

        // Clean up: close file descriptors and remove temporary files
        close(tar_fd);
        remove("temp.tar.gz");
        remove(temp_file_list);
    }

    void handle_filename_search(int client_fd, char *filename) {
        char response[1024];
        char path_buffer[2048] = {0}; // Initialize buffer to zero
        char command[1024];
        struct stat file_stat;
        struct tm timeinfo;
        struct tm *safe_timeinfo;
        int required_length;

        // Check for invalid characters in the filename
        if (strchr(filename, '/') != NULL) {
            write(client_fd, "Invalid file name\n", 18);
            return;
        }

        // command REGEX to find the file
        snprintf(command, sizeof(command), "find ~ -type f -name \"%s\" -print -quit", filename);
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            write(client_fd, "Error executing find command\n", 29);
            return;
        }

        // Retrieve the file path from the command output
        if (fgets(path_buffer, sizeof(path_buffer), fp) != NULL) {
            size_t len = strlen(path_buffer);
            if (len > 0 && path_buffer[len - 1] == '\n') {
                path_buffer[len - 1] = '\0'; // Remove the newline at the end
            }

            // Get file statistics and format the response
            if (stat(path_buffer, &file_stat) == 0) {
                safe_timeinfo = localtime_r(&file_stat.st_mtime, &timeinfo); // Thread-safe alternative to localtime
                char time_buffer[80];
                strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", safe_timeinfo);


            required_length = snprintf(NULL, 0, "File: %s\nSize: %ld bytes\nCreated: %s\nPermissions: %o\n",
                           path_buffer, file_stat.st_size, time_buffer, file_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));

            if (required_length >= sizeof(response)) {
            write(client_fd, "Error: Data too large for buffer\n", 33);
            return;
            }

                snprintf(response, sizeof(response),
                        "File: %s\nSize: %ld bytes\nCreated: %s\nPermissions: %o\n ",
                        path_buffer, file_stat.st_size, time_buffer, file_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
                write(client_fd, response, strlen(response));
            
            } else {
                write(client_fd, "File found but cannot retrieve details\n", 39);
            }
        } else {
            write(client_fd, "File not found\n", 16);
        }

        pclose(fp);
    }

    void handle_file_size_request(int client_fd, char *command) {
        long size1, size2;
        sscanf(command, "w24fz %ld %ld", &size1, &size2);

        if (size1 > size2 || size1 < 0 || size2 < 0) {
            write(client_fd, "Invalid size range\n", 19);
            return;
        }

        char *temp_file_list = "temp_file_list.txt";
        char find_command[1024];

        // Use 'find' command to locate files within size range, excluding hidden directories and files
        snprintf(find_command, sizeof(find_command), 
            "find ~ -type f -size +%ldc -size -%ldc \\( ! -regex '.*/\\..*' \\) > %s", size1, size2, temp_file_list);
            //  "cd ~ && find . -type f -not \\( -path './.*' -prune \\) -size +%ldc -size -%ldc > %s", size1, size2, temp_file_list);
        system(find_command); // Execute find command


        struct stat statbuf;
        // Check if the temporary file list is empty or does not exist
        if (stat(temp_file_list, &statbuf) != 0 || statbuf.st_size == 0) {
            write(client_fd, "No file found\n", 14);
            remove(temp_file_list);
            return;
        }

        char tar_command[1024];
        // Create a tarball of the files found
        snprintf(tar_command, sizeof(tar_command), "tar -czf temp.tar.gz -T %s 2>/dev/null", temp_file_list);

        system(tar_command);

        // Open the tarball to read its contents
        int tar_fd = open("temp.tar.gz", O_RDONLY);
        if (tar_fd < 0) {
            write(client_fd, "No file found\n", 14);
            remove(temp_file_list);
            return;
        }

        // Verify that the tar file was successfully created and is not empty
        if (fstat(tar_fd, &statbuf) != 0 || statbuf.st_size == 0) {
            write(client_fd, "No file found\n", 14);
            close(tar_fd);
            remove(temp_file_list);
            return;
        }
        
        // Send the tarball file size to the client
        char file_size_msg[64];
        snprintf(file_size_msg, sizeof(file_size_msg), "Filesize %ld\n", statbuf.st_size);
        write(client_fd, file_size_msg, strlen(file_size_msg));

        // Send the tarball content to the client
        char file_buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(tar_fd, file_buffer, sizeof(file_buffer))) > 0) {
            write(client_fd, file_buffer, bytes_read);
        }       

        // Cleanup
        close(tar_fd);
        remove("temp.tar.gz"); // Clean up the tarball
        remove(temp_file_list); // Clean up the file list
    }

    int creation_time_compare(const void *a, const void *b) {
        const DirEntry *dir_a = (const DirEntry *)a;
        const DirEntry *dir_b = (const DirEntry *)b;
        return (dir_a->creation_time > dir_b->creation_time) - (dir_a->creation_time < dir_b->creation_time);
    }

    int case_insensitive_compare(const void *a, const void *b) {
        const DirEntry *dir_a = (const DirEntry *)a;
        const DirEntry *dir_b = (const DirEntry *)b;
        return strcasecmp(dir_a->name, dir_b->name);
    }

void list_directories(int client_fd, const char *command) {
    DIR *dir;
    struct dirent *entry;
    DirEntry *dir_entries = NULL;
    size_t dir_count = 0;
    struct stat dir_stat;
    
    // Attempt to retrieve the home directory from environment variables
    char *home_directory = getenv("HOME");
    if (home_directory == NULL) {
        write(client_fd, "Failed to get home directory\n", 29);
        return;
    }

    dir = opendir(home_directory);
    if (dir == NULL) {
        perror("Failed to open home directory");
        write(client_fd, "Failed to open home directory\n", 30);
        return;
    }
    // Read entries from the home directory
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", home_directory, entry->d_name);
            
            if (stat(full_path, &dir_stat) == -1) {
                continue;
            }
            // Allocate or expand memory to store directory entries
            DirEntry *temp = realloc(dir_entries, (dir_count + 1) * sizeof(DirEntry));
            if (temp == NULL) {
                write(client_fd, "Memory allocation error\n", 24);
                closedir(dir);
                free(dir_entries); // Free any previously allocated memory
                return;
            }
            dir_entries = temp;
            dir_entries[dir_count].name = strdup(entry->d_name);
            dir_entries[dir_count].creation_time = dir_stat.st_mtime;
            dir_count++;
        }
    }
    closedir(dir);
    
    // Write sorted/filtered directory names to the client
    if (strcmp(command, "dirlist -t") == 0) {
        qsort(dir_entries, dir_count, sizeof(DirEntry), creation_time_compare);
    } else if (strcmp(command, "dirlist -a") == 0) {
        qsort(dir_entries, dir_count, sizeof(DirEntry), case_insensitive_compare);
    }

    for (size_t i = 0; i < dir_count; ++i) {
        write(client_fd, dir_entries[i].name, strlen(dir_entries[i].name));
        write(client_fd, "\n", 1);
    }
    write(client_fd, "\0", 1);  // Send an end-of-message delimiter

    free(dir_entries);
}

int connection_counts[3] = {0}; // Serverw24, mirror1, mirror2

// Server addresses for redirection
const char* server_addresses[2] = {"137.207.82.53", "137.207.82.53"};
int server_ports[2] = {4012, 4013}; // Assuming ports for mirror1 and mirror2

void redirect_client(int client_fd, const char *ip, int port) {
    char message[256];
    snprintf(message, sizeof(message), "Please connect to server: %s:%d\n", ip, port);
    send(client_fd, message, strlen(message), 0);
    close(client_fd);
}

int determine_server(int connection_number) {
    if (connection_number <= 3) {
        return -1;  // serverw24 (main server)
    } else if (connection_number <= 6) {
        return 0;   // mirror1
    } else if (connection_number <= 9) {
        return 1;   // mirror2
    } else {
        return (connection_number - 10) % 3 - 1; // Cycle through servers
    }
}

  void process_client_request(int client_fd) {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);  // Ensure buffer is clean before reading
        ssize_t read_bytes = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (read_bytes <= 0) {
            if (read_bytes == 0) {
                printf("Client disconnected\n");
            } else {
                perror("Error reading from socket");
            }
            break;
        }

        buffer[read_bytes] = '\0'; // Null-terminate the command string

        // Handle commands
        if (strcmp(buffer, "quitc") == 0) {
            break; // Client requested to quit
        } else if (strncmp(buffer, "dirlist", 7) == 0) {
            list_directories(client_fd, buffer);
        } else if (strncmp(buffer, "w24fz", 5) == 0) {
            handle_file_size_request(client_fd, buffer);
        } else if (strncmp(buffer, "w24fn", 5) == 0) {
            handle_filename_search(client_fd, buffer + 6);
        } else if (strncmp(buffer, "w24ft", 5) == 0) {
            handle_file_type_request(client_fd, buffer + 6);
        } else if (strncmp(buffer, "w24fdb", 6) == 0) {
            handle_find_before_date_request(client_fd, buffer + 7);
        } else if (strncmp(buffer, "w24fda", 6) == 0) {
            handle_find_after_date_request(client_fd, buffer + 7);
        }
    }

    close(client_fd);
}

void handle_connection(int client_fd, int connection_number) {
    int server_idx = determine_server(connection_number);

    if (server_idx == -1) {
        // Handle on this server
        connection_counts[0]++;
        printf("Handling connection %d on main server\n", connection_number);
        process_client_request(client_fd);
    } else {
        // Redirect to mirror server
        redirect_client(client_fd, server_addresses[server_idx], server_ports[server_idx]);
        connection_counts[server_idx + 1]++;
        // printf("Redirecting connection %d to mirror%d\n", connection_number, server_idx + 1);
    }
}

    int main() {
        int sockfd, new_sock;  // Socket file descriptors
        struct sockaddr_in server_addr, client_addr; // Socket address structures
        socklen_t cli_len = sizeof(client_addr);
        int connection_number = 0;

        // Create a socket (IPv4, TCP)
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("ERROR while opening socket");
            exit(1);
        }

        // the sockaddr_in structure
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;   // Bind to all available interfaces
        server_addr.sin_port = htons(PORT);    // Server port

        if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("ERROR on binding");
            exit(1);
        }

        // Listen for incoming connections
        listen(sockfd, 5);
        printf("Server listening on port %d\n", PORT);

        while (1) {
            // Accept an incoming connection
            new_sock = accept(sockfd, (struct sockaddr *)&client_addr, &cli_len);
            if (new_sock < 0) {
                perror("ERROR on accept");
                continue;
            }
            connection_number++;

            if (!fork()) {   
                close(sockfd);
                handle_connection(new_sock, connection_number); // Handle or redirect the connection'
                close(new_sock); // Close the client socket after handling
                exit(0);
            } else {
                close(new_sock);    // Parent does not need this socket
            }
        }

        return 0;
    }
