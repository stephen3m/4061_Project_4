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

int send_file(int socket, FILE *fd) {
    // Send the file data
    char msg[8]; // to store image data
    memset(msg, 0, 8);
    printf("begin sending\n");
    while (1) {
        int bytes_read = fread(msg, sizeof(char), 8, fd);
        if (bytes_read == 0)
            break;
        else if (bytes_read < 0)
            perror("fread failed");
        
        int bytes_sent = 0;
        bytes_sent = send(socket, msg, bytes_read, 0);
        if(bytes_sent == -1) // send message to server and error check
            perror("send error");
        memset(msg, 0, 8); // clear buffer for next read
        
        printf("sending data %d and read %d\n", bytes_sent, bytes_read);
        // wait for server to send ack before continuing to send more data
        char received_data[PACKETSZ];
        memset(received_data, 0, PACKETSZ);
        if (recv(socket, received_data, PACKETSZ, 0) == -1)
            perror("recv error");
        printf("receiving ack packet\n");
        
        packet_t *received_packet = NULL;
        received_packet = deserializeData(received_data);
        if (received_packet->operation == IMG_OP_NAK) {// if not acknowledge, skip image
            printf("err occurred\n");
            return -3;
        }

        free(received_packet);
        printf("perpared to send next chunk\n");
    }

    printf("done sending\n");
    
    if(fclose(fd) != 0) 
        perror("fclose failed");

    return 0;
}

int receive_file(int socket, const char *filename) {
    // Convert ./img/x/xx.png to ./output/x/xx.png
    char *fname = strstr(filename, "img"); // gets pointer to "img" in filename
    char output_file[BUFF_SIZE];
    memset(output_file, 0, BUFF_SIZE);
    if (fname != NULL) {
        sprintf(output_file, "./output/%s", fname + strlen("img")); // E.g. output_file would be "./output/0/4511.png" if fname is "img/0/4511.png"
    } else {
        perror("Substring 'img' not found in filename");
    }

    // Open the file
    FILE *fd = fopen(output_file, "wb");
    if (fd == NULL)
        perror("Error opening file");

    // for receiving the response packet
    char received_data[8];
    memset(received_data, 0, 8);

    // create acknowledge packet for later
    packet_t ack_packet = {IMG_OP_ACK, 0, htons(0)};
    char *serialized_ack = serializePacket(&ack_packet);
    printf("begin receiving\n");
    while(1) {
        // read in data from server
        int bytes_read = recv(socket, received_data, 8, 0);
        printf("received %d bytes\n", bytes_read);
        if (bytes_read == -1)
            perror("recv error");
        else if (bytes_read == 1 && !strcmp(received_data, "f"))
            break;
        else if (bytes_read == 1)
            break;

        fwrite(received_data, sizeof(char), bytes_read, fd); 
        memset(received_data, 0, 8);
        printf("received and wrote data %d\n", bytes_read);
        // send back ack packet to let server know we got the data
        if(send(socket, serialized_ack, PACKETSZ, 0) == -1) 
            break;
        printf("send back ack packet\n");
    }
    printf("done reading data\n");

    free(serialized_ack);
    if(fclose(fd) != 0) 
        perror("fclose failed");

    return 0;
}

int main(int argc, char* argv[]) {
    if(argc != 4){
        fprintf(stderr, "Usage: ./client File_Path_to_images File_Path_to_output_dir Rotation_angle. \n");
        return 1;
    }

    // Store main arguments in variables
    char file_path[BUFF_SIZE];
    memset(file_path, 0, BUFF_SIZE);
    strcpy(file_path, argv[1]);
    
    char direct_path[BUFF_SIZE];
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
            sprintf(full_file_path, "%s/%s", file_path, entry->d_name);
            enqueue_request(angle, full_file_path); // synchronization handled in enqueue_request
        }
    }


    while(1) {// while queue !empty
        // send image metadata
        // Set up the request packet for the server and send it
        request_t *cur_request = dequeue_request();

        if (cur_request == NULL) // if all images have been dequeued, break
            break;
        
        // Open the file
        // printf("%s\n", cur_request->file_name);
        FILE *fd = fopen(cur_request->file_name, "r");
        if (fd == NULL) 
            perror("Error opening file for sending");

        fseek(fd, 0, SEEK_END); // calculate size of image and set packet accordingly
        int size = ftell(fd);
        fseek(fd, 0, SEEK_SET); // return file pointer to beginning of file

        int flag = 0;
        if(cur_request->rotation_angle == 180)
            flag = IMG_FLAG_ROTATE_180;
        else if(cur_request->rotation_angle == 270)
            flag = IMG_FLAG_ROTATE_270;
        
        packet_t request_packet = {IMG_OP_ROTATE, flag, size};
        // serialize and send packet w/ img info to clientHandler
        char *serialized_data = serializePacket(&request_packet);

        printf("Sending metapacket\n");
        if (send(sockfd, serialized_data, PACKETSZ, 0) == -1)
            perror("send error");

        // wait for ack package before sending image data
        // received_data will be acknowledgement packet from server
        char received_data[PACKETSZ];
        memset(received_data, 0, PACKETSZ);
        if (recv(sockfd, received_data, PACKETSZ, 0) == -1)
            perror("recv error");
        printf("Received response packet\n");
        
        packet_t *received_packet = NULL;
        received_packet = deserializeData(received_data);
        if (received_packet->operation == IMG_OP_NAK) // if not acknowledge, skip image
            continue;

        // Send the image data to the server
        printf("entering send func\n");
        int send_results = send_file(sockfd, fd);
        if (send_results == -3) // if IMG_OP_NAK was received, skip this image & continue with rest of queue
            continue;

        printf("exiting send func good and entering receive\n");
        // Receive the processed image and save it in the output dir
        receive_file(sockfd, cur_request->file_name);
        printf("exiting receive file\n");

        free(serialized_data);
        free(received_packet);
        free(cur_request);
    }

    // send clientHandler IMG_OP_EXIT to have it terminate connection
    packet_t request_packet = {IMG_OP_EXIT, 0, htons(0)};
    char* serialized_data = serializePacket(&request_packet);
    if (send(sockfd, serialized_data, PACKETSZ, 0) == -1)
        perror("send error");

    // Terminate the connection once all images have been processed
    close(sockfd);
    // Close opened directories
    closedir(file_directory);
    // Free as needed
    free(serialized_data);

    return 0;
}
