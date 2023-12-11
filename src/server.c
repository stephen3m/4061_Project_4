#include "server.h"

#define PORT 4891
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024 

// Global vars
pthread_t workerArray[BUFFER_SIZE];
int worker_idx = 0;
int workers_done = 0;

void *clientHandler(void *input_socket) {
    // Create error packet: used to send IMG_OP_NAK
    packet_t error_packet = {IMG_OP_NAK, IMG_FLAG_ENCRYPTED, htonl(0)};
    char *serialized_error = serializePacket(&error_packet);

    // Create ack packet: use to send IMG_OP_ACK
    packet_t ack_packet = {IMG_OP_ACK, IMG_FLAG_ENCRYPTED, htonl(0)};
    char *serialized_ack = serializePacket(&ack_packet);
    
    // filename is the name of the temp file we will be writing the inital received data from client to
    // rot_file will be the name of the temp file we will write to with stbi_write, stores rotated image data
    // we will then read rot_file and send the byte stream over to client like how client sent data to server 
    char filename[BUFF_SIZE]; 
    char rotated_file[BUFFER_SIZE];
    memset(filename, 0, BUFF_SIZE);
    memset(rotated_file, 0, BUFF_SIZE);

    sprintf(filename, "temp_file%ld", pthread_self()); // will be smth like temp_file1 or 2 or 3...
    sprintf(rotated_file, "rotated_image_data%ld", pthread_self()); // rotated_image_data1 or 2 or 3...

    int *socket = (int*) input_socket;

    FILE *fd = fopen(filename, "r");
    if (fd == NULL) {
        // send IMG_OP_NAK, don't need to call exit since client will handle that when it receives IMG_OP_NAK
        send(*socket, serialized_error, sizeof(packet_t), 0);
    }
    
    FILE *rot_fd = fopen(rotated_file, "w");
    if (rot_fd == NULL) {
        send(*socket, serialized_error, sizeof(packet_t), 0);
    }

    while(1) {
        // Receive packet from the client
        char recvdata[PACKETSZ];
        memset(recvdata, 0, PACKETSZ);
        int ret = recv(*socket, recvdata, PACKETSZ, 0);
        if(ret == -1) {
            send(*socket, serialized_error, sizeof(packet_t), 0); 
        }

        // Determine the packet operatation and flags, send acknowledgement
        packet_t *recvpacket = deserializeData(recvdata);
        int angle;
        if (recvpacket->operation == IMG_OP_EXIT) // received IMG_OP_EXIT from client
            break;
        else if (recvpacket->operation == IMG_OP_ROTATE) {
            if (send(*socket, serialized_ack, sizeof(packet_t), 0) == -1)
                send(*socket, serialized_error, sizeof(packet_t), 0);
            if (recvpacket->flags == IMG_FLAG_ROTATE_180)
                angle = 180;
            else if (recvpacket->flags == IMG_FLAG_ROTATE_270)
                angle = 270;
        }

        // Receive the image data using the size
        long total_bytes_read = 0;
        int bytes_read = 0;
        char imgData[BUFF_SIZE + 1]; // +1 for null terminator
        memset(imgData, 0, BUFF_SIZE + 1);
        while(total_bytes_read < recvpacket->size) {
            // get data from client
            bytes_read = recv(*socket, imgData, BUFF_SIZE, 0);
            if (bytes_read == -1)
                send(*socket, serialized_error, sizeof(packet_t), 0);
            
            // write received data to temp file
            printf("%s\n", imgData);
            fwrite(imgData, sizeof(char), BUFF_SIZE, fd);
            // prepare for next iteration
            total_bytes_read += bytes_read;
            memset(imgData, 0, BUFF_SIZE + 1);

            // send back ack packet to client for sending next data chunk
            if (send(*socket, serialized_ack, sizeof(packet_t), 0) == -1)
                send(*socket, serialized_error, sizeof(packet_t), 0);
        }

        printf("death\n");

        // Process the image data based on the set of flags
        // Stbi_load loads in an image from specified location; populates width, height, and bpp with values
        int width;
        int height;
        int bpp;
        uint8_t* image_result = stbi_load(filename, &width, &height, &bpp, CHANNEL_NUM);

        uint8_t **result_matrix = (uint8_t **)malloc(sizeof(uint8_t*) * width);
        uint8_t **img_matrix = (uint8_t **)malloc(sizeof(uint8_t*) * width);
        for(int i = 0; i < width; i++){
            result_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
            img_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
        }

        linear_to_image(image_result, img_matrix, width, height);
        
        if (angle == 180)
            flip_left_to_right(img_matrix, result_matrix, width, height);
        else if (angle == 270) 
            flip_upside_down(img_matrix, result_matrix, width, height);

        uint8_t* img_array = malloc(sizeof(uint8_t) * (width) * (height)); ///Hint malloc using sizeof(uint8_t) * width * height

        flatten_mat(result_matrix, img_array, width, height);

        //You should be ready to call stbi_write_png using:
        //New path to where you wanna save the file, Width, height, img_array, width*CHANNEL_NUM
        stbi_write_png(rotated_file, width, height, CHANNEL_NUM, img_array, (width) * CHANNEL_NUM);

        // Free mallocs and set to NULL to avoid double frees
        for(int i = 0; i < width; i++){
            free(result_matrix[i]);
            free(img_matrix[i]);
            result_matrix[i] = NULL;
            img_matrix[i] = NULL;
        }

        free(result_matrix);
        free(img_matrix);
        free(img_array);
        free(image_result);
        image_result = NULL;
        result_matrix = NULL;
        img_matrix = NULL;
        img_array = NULL;

        
        // Send rotated_file data to client
        char msg[BUFF_SIZE]; // to store image data
        memset(msg, 0, BUFF_SIZE); 
        while (fread(msg, sizeof(char), BUFF_SIZE, rot_fd) > 0) { 
            // send image data
            if(send(*socket, msg, BUFF_SIZE, 0) == -1) // send message to server and error check
                send(*socket, serialized_error, sizeof(packet_t), 0);
            memset(msg, 0, BUFF_SIZE); // clear buffer for next read
        }

        // once done sending image data, send "END" to client to signal EOF
        strcpy(msg, "END");
        if(send(*socket, msg, BUFF_SIZE, 0) == -1) // send message to server and error check
                send(*socket, serialized_error, sizeof(packet_t), 0);
    }

    if (fclose(fd) == -1)
        perror("issue closing temp file");
    if (fclose(rot_fd) == -1)
        perror("issue closing temp rot file");

    // delete temp files
    if (remove(filename) != 0)
        perror("Issue with deleting temp file");
    if (remove(rotated_file) != 0)
        perror("Issue with deleting temp rot file");
    
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    // Creating socket file descriptors
    int listen_fd, conn_fd;

    // Create listening socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1)
        perror("socket error");

    // Populating servaddr fields
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen to any of the network interface (INADDR_ANY)
    servaddr.sin_port = htons(PORT); // Port number

    // Bind the socket to the port
    int ret = bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)); // bind address, port to socket
    if(ret == -1){
        perror("bind error");
    }
        
    // Listen on the socket
    ret = listen(listen_fd, MAX_CLIENTS); // Stephen: If we are running 11 clients, is it ok that MAX_CLIENTS IS 5?
    if(ret == -1){
        perror("listen error");
    }

    // Accept connections and create the client handling threads
    while(1) {
        struct sockaddr_in clientaddr;
        socklen_t clientaddr_len = sizeof(clientaddr);
        // accept a request from a client
        conn_fd = accept(listen_fd, (struct sockaddr *) &clientaddr, &clientaddr_len); 
        if(conn_fd == -1) {
            close(conn_fd); 
            close(listen_fd);
            perror("accept encountered error");
        }

        // // create a handling thread for the client
        // if(pthread_create(&workerArray[worker_idx], NULL, clientHandler, (void *)&conn_fd) != 0){
        //     close(conn_fd); 
        //     close(listen_fd);
        //     perror("Error creating worker thread");
        // }
        // pthread_detach(workerArray[worker_idx]);
        // worker_idx++;

        // debugging by running clientHandler once
        clientHandler((void *)&conn_fd);
    }

    return 0;
}
