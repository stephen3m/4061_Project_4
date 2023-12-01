#include "client.h"

#define PORT 4891
#define BUFFER_SIZE 1024 

int send_file(int socket, const char *filename) {
    // Open the file
    FILE *fd = fopen(filename, "r");
    if (fd == NULL) {
        perror("Error opening file");
        return -1;
    }

    // Set up the request packet for the server and send it
    packet_t request_packet;
    // request_packet.operation = 
    // request_packet.flags =
    // request_packet.size = 
    send(socket, &request_packet, sizeof(packet_t), 0);

    // Send the file data
    
    fclose(fd);
    return 0;
}

int receive_file(int socket, const char *filename) {
    // Open the file
    FILE *fd = fopen(filename, "w");
    if (fd == NULL) {
        perror("Error opening file");
        return -1;
    }

    // Receive response packet
    packet_t response_packet;
    recv(socket, &response_packet, sizeof(packet_t), 0);

    // Receive the file data

    // Write the data to the file

    fclose(fd);
    return 0;
}

int main(int argc, char* argv[]) {
    if(argc != 4){
        fprintf(stderr, "Usage: ./client File_Path_to_images File_Path_to_output_dir Rotation_angle. \n");
        return 1;
    }

    char direct_path = [BUFF_SIZE];
    memset(direct_path, 0, BUFF_SIZE);
    strcpy(direct_path, argv[2]);
    int angle = atoi(argv[3]);
    
    // Set up socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // create socket to establish connection
    if(sockfd == -1)
        perror("socket error");

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // server IP, since the server is on same machine, use localhost IP
    servaddr.sin_port = htons(PORT); // Port the server is listening on

    // Connect the socket
    int ret = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)); // establish connection to server
    if(ret == -1)
        perror("connect error");

    // Read the directory for all the images to rotate
    // Open the file path to output directory
    // Check if opendir is successful
    DIR* output_directory = opendir(direct_path);
    if(output_directory == NULL){
        fprintf(stderr, "Invalid output directory\n");
        return -1;
    }

    request_t *current_node = malloc(sizeof(request_t));
    request_t *previous_node = NULL;
    request_t *head_node = current_node;

    struct dirent *entry;
    while ((entry = readdir(output_directory)) != NULL) {
        // Skip the "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // Enqueue for ".png" files
        char *extension = strrchr(entry->d_name, '.'); // gets pointer to last occurrence of "." in entry->d_name
        if (extension != NULL && strcmp(extension, ".png") == 0) { // check if file ends in ".png"
            char file_path[BUFF_SIZE]; // in the form "img/x/xxx.png"
            memset(file_path, 0, BUFF_SIZE);
            sprintf(file_path, "%s/%s", direct_path, entry->d_name);

            strcpy(current_node->file_name, file_path);
            current_node->rotation_angle = angle;
            current_node->next_node = malloc(sizeof(request_t));
            current_node->prev_node = previous_node;

            previous_node = current_node;
            current_node = current_node->next_node;
            
        }
    }
    // no more files, set end of queue's next to NULL
    request_t *queue_end = current_node;
    current_node->next_node = NULL;


    // Send the image data to the server
    /* Stephen: Here's a piazza post reply: 
    You can read an image into a buffer in the client. 
    Then, send the buffer directly to the server. 
    The server then creates a temporary file to store the buffer. 
    Now, you can use stbi_laod() and any other operations we did in PA3 to rotate the image.
    */

    current_node = head_node;
    while(current_node != NULL) {
        // Check that the request was acknowledged
        send_file(sockfd, current_node->file_name);


        // Receive the processed image and save it in the output dir
        receive_file(sockfd, current_node->file_name);

        current_node = current_node->next_node;
    }

    // Terminate the connection once all images have been processed
    close(sockfd); // close socket

    // Release any resources
    current_node = queue_end;
    while(current_node != NULL) {
        current_node = current_node->prev_node;
        free(current_node->next_node);
    }
    free(head_node);

    return 0;
}
