#include "client.h"

#define PORT 4891
#define BUFFER_SIZE 1024 

// global vars
request_t *req_queue = NULL;
request_t *end_queue = NULL;

request_t *dequeue_request() {
    if (req_queue == NULL) {
        return NULL; // Queue is empty
    }
    request_t *ret_request = req_queue;
    req_queue = req_queue->next;

    if (req_queue == NULL) { // Queue had one object which was dequeued
        end_queue = NULL; 
    }
    return ret_request;
}

void enqueue_request(int new_angle, char* file_path){

    request_t *new_request = malloc(sizeof(request_t));
    if (new_request == NULL) { // Malloc error
        return;
    }
    strcpy(new_request->file_path, file_path);
    new_request->angle = new_angle;
    new_request->next = NULL;

    if (req_queue == NULL){
        req_queue = new_request;
        end_queue = new_request;
    }
    else{
        end_queue->next = new_request;
        end_queue = end_queue->next;
    }
}

int send_file(int socket, const char *filename) {
    // Open the file
    FILE *fd = fopen(filename, "r");
    if (fd == NULL) {
        perror("Error opening file");
        return -1; 
    }

    // Set up the request packet for the server and send it
    request_t *cur_request = dequeue_request();

    if (cur_request == NULL) // if end of queue (null), return -2
        return -2;

    packet_t request_packet;
    request_packet.operation = IMG_OP_ROTATE;
    if(cur_request->rotation_angle == 180){
        request_packet.flags = IMG_FLAG_ROTATE_180;
    }
    else if(cur_request->rotation_angle == 270){
        request_packet.flags = IMG_FLAG_ROTATE_270;
    }
    // request_packet.flags =
    send(socket, &request_packet, sizeof(packet_t), 0); // do we need to error check this?

    // read in image data from file
    char msg[size]; // to store all of the image data
    char buffer[BUFF_SIZE]; // 
    memset(buffer, 0, BUFF_SIZE);
    memset(msg, size); // initialize msg with '\0'
    while (read(fd, buffer, BUFF_SIZE) > 0) { // make sure to read in all data from file
        strcat(msg, buffer);
        memset(buffer, 0, BUFF_SIZE);
    }

    // send image data
    setbuf(stdin, NULL);
    if(send(socket, msg, size, 0) == -1) // send message to server and error check
        perror("send error");
    
    fclose(fd);
    return 0;
}

int receive_file(int socket, const char *filename) {
    // need filename in /output instead of /img [turning /img/.../file.png -> /output/.../file.png]
    // extract the actual filename out of *filename
    char *fname = strrchr(filename, "img"); // gets pointer to last occurrence of "img" from *filename
    char output_file[BUFF_SIZE];
    sprintf(output_file, "/output%s", fname);

    // Open the file
    FILE *fd = fopen(output_file, "w");
    if (fd == NULL) {
        perror("Error opening file");
        return -1;
    }

    // Receive response packet
    packet_t response_packet;
    recv(socket, &response_packet, sizeof(packet_t), 0);

    // Receive the file data from clientHandler
    int size = response_packet->size;
    char msg[size]; // to store rotated image data
    bzero(msg, size); // initialize msg with '\0'
    if (recv(socket, msg, size, 0) == -1) // receive rotated image data and error check
        perror("recv error");

    // Write the data to the file
    while()?


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
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // create socket to establish connection
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

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip the "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // Enqueue for ".png" files
        char *extension = strrchr(entry->d_name, '.'); // gets pointer to last occurrence of "." in entry->d_name
        if (extension != NULL && strcmp(extension, ".png") == 0) { // check if file ends in ".png"
            char file_path[BUFF_SIZE*2]; // in the form "img/x/xxx.png"
            memset(file_path, 0, BUFF_SIZE*2);
            sprintf(file_path, "%s/%s", directory_path, entry->d_name);
            enqueue_request(angle, file_path); // synchronization handled in enqueue_request
            
        }
    }

    // Send the image data to the server
    /* Stephen: Here's a piazza post reply: 
    You can read an image into a buffer in the client. 
    Then, send the buffer directly to the server. 
    The server then creates a temporary file to store the buffer. 
    Now, you can use stbi_laod() and any other operations we did in PA3 to rotate the image.
    */

    // while(1) {
    //     // Check that the request was acknowledged
    //     int send_results = send_file(sockfd, current_node->file_name);
    //     if (send_results == -2)
    //         break; // if reached end of queue, break from loop
    //     else if (send_results == -1) // how to handle unable to open file?
    //         continue;

    //     // Receive the processed image and save it in the output dir
    //     receive_file(sockfd, current_node->file_name);

    //     current_node = current_node->next_node;
    // }

    // INTER SUBMISSION
    packet_t request_packet = {IMG_OP_ROTATE, '', htons(0)}
    char *serializedData = serializePacket(&request_packet);
    if (send(sockfd, serializedData, sizeof(packet_t), 0) == -1)
        perror("send error\n");

    // Terminate the connection once all images have been processed
    // request_packet = packet_t request_packet = {IMG_OP_ROTATE, '', htons(0)}
    // send(sockfd, &request_packet, sizeof(packet_t), 0);

    close(sockfd); // close socket

    // Release any resources
    current_node = queue_end;
    while(current_node != NULL) {
        current_node = current_node->prev_n;
        free(current_node->next_node);
    }
    free(head_node);

    return 0;
}
