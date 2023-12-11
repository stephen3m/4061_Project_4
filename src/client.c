#include "client.h"

#define PORT 4891
#define BUFFER_SIZE 1024 

// Global vars
request_t *req_queue = NULL;
request_t *end_queue = NULL;

request_t *dequeue_request() {
    if (req_queue == NULL) {
        return NULL; // Queue is empty
    }
    request_t *ret_request = req_queue;
    req_queue = req_queue->next_node;

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
    strcpy(new_request->file_name, file_path);
    new_request->rotation_angle = new_angle;
    new_request->next_node = NULL;

    if (req_queue == NULL){
        req_queue = new_request;
        end_queue = new_request;
    }
    else{
        end_queue->next_node = new_request;
        end_queue = end_queue->next_node;
    }
}

// returns -2 when end of queue is reached; returns -3 if IMG_OP_NAK was received by server
int send_file(int socket, char *filename) {
    // Open the file
    // printf("%s\n", filename);
    FILE *fd = fopen(filename, "r");
    if (fd == NULL) 
        perror("Error opening file for sending");

    // Set up the request packet for the server and send it
    request_t *cur_request = dequeue_request();

    if (cur_request == NULL) // if end of queue (null), return -2
        return -2;

    packet_t request_packet;
    request_packet.operation = IMG_OP_ROTATE;
    if(cur_request->rotation_angle == 180)
        request_packet.flags = IMG_FLAG_ROTATE_180;
    else if(cur_request->rotation_angle == 270)
        request_packet.flags = IMG_FLAG_ROTATE_270;

    fseek(fd, 0, SEEK_END); // calculate size of image and set packet accordingly
    request_packet.size = htonl(ftell(fd));
    fseek(fd, 0, SEEK_SET); // return filepointer to beginning for reading later

    // serialize and send packet to clientHandler
    char *serialized_data = serializePacket(&request_packet);
    if (send(socket, serialized_data, sizeof(packet_t), 0) == -1)
        perror("send error");

    // wait to receive acknowledge packet before sending image data
    char received_data[sizeof(char) * PACKETSZ];
    memset(received_data, 0, sizeof(char) * PACKETSZ);
    if (recv(socket, received_data, sizeof(packet_t), 0) == -1)
        perror("recv error");
    
    packet_t *received_packet = deserializeData(received_data);
    if (received_packet->operation == IMG_OP_NAK)
        return -3; // Server did not acknowledge image, skip this one
    free(received_packet);

    // read chunks of image data from file into buffer and send to server (clientHandler) 
    char msg[BUFF_SIZE + 1]; // to store image data
    memset(msg, 0, BUFF_SIZE + 1); 
    while (fread(msg, sizeof(char), BUFF_SIZE, fd) > 0) { 
        // send image data
        printf("%s\n", msg);
        if(send(socket, msg, BUFF_SIZE, 0) == -1) // send message to server and error check
            perror("send error");
        memset(msg, 0, BUFF_SIZE + 1); // clear buffer for next read

        // only continue if server sends back ack
        memset(received_data, 0, PACKETSZ);
        if (recv(socket, received_data, sizeof(packet_t), 0) == -1)
            perror("recv error");
        
        received_packet = deserializeData(received_data);

        if (received_packet->operation == IMG_OP_NAK)
            return -3; // Server did not acknowledge image, skip this one
        free(received_packet);
    }
    
    printf("death\n");

    free(serialized_data);
    free(cur_request);
    received_packet = NULL;
    serialized_data = NULL;
    cur_request = NULL;
    fclose(fd);
    return 0;
}

int receive_file(int socket, const char *filename) {
    // we want to convert ./img/x/xx.png to ./output/x/xx.png
    char *fname = strstr(filename, "img"); // gets pointer to "img" in filename
    char output_file[BUFF_SIZE];
    memset(output_file, 0, BUFF_SIZE);
    if (fname != NULL) {
        sprintf(output_file, "./output/%s", fname + strlen("img")); // E.g. output_file would be "./output/0/4511.png" if fname is "img/0/4511.png"
    } else {
        perror("Substring 'img' not found in filename");
    }

    // Open the file
    FILE *fd = fopen(output_file, "w");
    if (fd == NULL)
        perror("Error opening file");
    
    char received_data[BUFF_SIZE + 1];
    memset(received_data, 0, BUFF_SIZE + 1);
    while(1) {
        // TODO: Just a question. Is received_data supposed to be the rotated_file file name OR "END"?
        // - it should be the rotated file DATA i.e. the bytes that make up the image itself
        // - otherwise, it will be END showing that the server has finished sending all the image data
        if (recv(socket, received_data, sizeof(packet_t), 0) == -1)
            perror("recv error");
        // TODO: sending "END" not implemented in server's clientHandler end yet
        if (!strcmp(received_data, "END"))
            break;
        // TODO: null terminator might cause bugs
        fwrite(received_data, sizeof(char), BUFF_SIZE, fd); 
        memset(received_data, 0, BUFF_SIZE);
    }

    fclose(fd);
    return 0;
}

int main(int argc, char* argv[]) {
    if(argc != 4){
        fprintf(stderr, "Usage: ./client File_Path_to_images File_Path_to_output_dir Rotation_angle. \n");
        return 1;
    }

    // Store main arguments in variables
    char direct_path[BUFF_SIZE];
    memset(direct_path, 0, BUFF_SIZE);
    strcpy(direct_path, argv[2]);
    int angle = atoi(argv[3]);
    char file_path[BUFF_SIZE];
    memset(file_path, 0, BUFF_SIZE);
    strcpy(file_path, argv[1]);
    
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
    DIR* file_directory = opendir(file_path);
    if(file_directory == NULL){
        fprintf(stderr, "Invalid output directory\n");
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(file_directory)) != NULL) {
        // Skip the "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // Enqueue for ".png" files
        char *extension = strrchr(entry->d_name, '.'); // gets pointer to last occurrence of "." in entry->d_name
        if (extension != NULL && strcmp(extension, ".png") == 0) { // check if file ends in ".png"
            char full_file_path[BUFF_SIZE*2]; // in the form "img/x/xxx.png"
            memset(full_file_path, 0, BUFF_SIZE*2);
            // printf("%s/%s", file_path, entry->d_name);
            sprintf(full_file_path, "%s/%s", file_path, entry->d_name);
            enqueue_request(angle, full_file_path); // synchronization handled in enqueue_request
        }
    }

    while(1) {
        // queue empty
        if (req_queue == NULL)
            break;

        // get path of current image
        char filename[BUFF_SIZE];
        memset(filename, 0, BUFF_SIZE);
        strcpy(filename, req_queue->file_name);

        // send metadata and image data to clientHandler
        int send_results = send_file(sockfd, filename);
        if (send_results == -2) // if reached end of queue, break from loop
            break;
        else if (send_results == -3) // Skip this image, continue with rest of queue
            continue;

        // Receive the processed image and save it in the output dir
        receive_file(sockfd, filename);
    }

    // tell clientHandler to die
    packet_t request_packet = {IMG_OP_EXIT, IMG_FLAG_CHECKSUM, htonl(0)};
    char* serialized_data = serializePacket(&request_packet);
    if (send(sockfd, serialized_data, sizeof(packet_t), 0) == -1)
        perror("send error");

    // Terminate the connection once all images have been processed
    close(sockfd);

    // Free mallocs and close opened directories
    closedir(file_directory);
    return 0;
}

    