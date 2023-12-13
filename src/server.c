#include "server.h"

#define PORT 4891
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024 


void *clientHandler(void *socket_fd) {
    int socket = *((int*) socket_fd);
    // Create error packet: used to send IMG_OP_NAK
    packet_t error_packet = {IMG_OP_NAK, 0, htons(0)};
    char *serialized_error = serializePacket(&error_packet);
    // // Create ack packet: use to send IMG_OP_ACK
    packet_t ack_packet = {IMG_OP_ACK, 0, htons(0)};
    char *serialized_ack = serializePacket(&ack_packet);

    // create temp files to write to
    char filename[BUFF_SIZE]; 
    char rotated_file[BUFFER_SIZE];
    memset(filename, 0, BUFF_SIZE);
    memset(rotated_file, 0, BUFF_SIZE);
    sprintf(filename, "temp_img%ld.png", pthread_self()); // filename will be temp_imgX where X is thread ID
    sprintf(rotated_file, "rot_temp_img%ld.png", pthread_self()); // filename rot_temp_imgX

    FILE *fd = fopen(filename, "w+");
    if (fd == NULL) {
        // send IMG_OP_NAK, don't need to call exit since client will handle that when it receives IMG_OP_NAK
        send(socket, serialized_error, PACKETSZ, 0);
    }
    FILE *rot_fd = fopen(rotated_file, "w+");
    if (rot_fd == NULL) {
        send(socket, serialized_error, PACKETSZ, 0);
    }

    while (1) {
        fd = fopen(filename, "w+");
        if (fd == NULL) {
            // send IMG_OP_NAK, don't need to call exit since client will handle that when it receives IMG_OP_NAK
            send(socket, serialized_error, PACKETSZ, 0);
        }
        rot_fd = fopen(rotated_file, "w+");
        if (rot_fd == NULL) {
            send(socket, serialized_error, PACKETSZ, 0);
        }
        
        // Receive initial metadata from the client
        char recvdata[PACKETSZ];
        memset(recvdata, 0, PACKETSZ);
        int ret = recv(socket, recvdata, PACKETSZ, 0);
        if(ret == -1) {
            send(socket, serialized_error, PACKETSZ, 0); 
        }
        packet_t *recvpacket = NULL;
        recvpacket = deserializeData(recvdata);
        
        // if packet op is EXIT, break from loop before doing anything else
        if (recvpacket->operation == IMG_OP_EXIT) {
            free(recvpacket);
            break;
        }

        // send back acknowledgement
        if(send(socket, serialized_ack, PACKETSZ, 0) == -1)
            send(socket, serialized_error, PACKETSZ, 0); 

        // Receive the image data using the size
        char received_data[BUFF_SIZE];
        memset(received_data, 0, BUFF_SIZE);
        int failed = 0;
        int total_bytes_read = 0;
        while (total_bytes_read < recvpacket->size) {
            // read in data from client
            int bytes_read = recv(socket, received_data, BUFF_SIZE, 0);
            if (bytes_read == -1)
                send(socket, serialized_error, PACKETSZ, 0);
            else if (bytes_read == 1 && bytes_read == 'f') // done reading data
                break;
            total_bytes_read += bytes_read;

            fwrite(received_data, sizeof(char), bytes_read, fd); 
            memset(received_data, 0, BUFF_SIZE);

            // send back ack packet to let client know we got the data
            if(send(socket, serialized_ack, PACKETSZ, 0) == -1) {
                failed = 1;
                break;
            }
        }

        if (failed) {
            send(socket, serialized_error, PACKETSZ, 0);
            continue;
        }
        
        // Process the image data based on the set of flags
        // Stbi_load loads in an image from specified location; populates width, height, and bpp with values
        rewind(fd);
        int width;
        int height;
        int bpp;
        uint8_t* image_result = stbi_load_from_file(fd, &width, &height, &bpp, CHANNEL_NUM);

        uint8_t **result_matrix = (uint8_t **)malloc(sizeof(uint8_t*) * width);
        uint8_t **img_matrix = (uint8_t **)malloc(sizeof(uint8_t*) * width);
        for(int i = 0; i < width; i++){
            result_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
            img_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
        }

        linear_to_image(image_result, img_matrix, width, height);
        
        if (recvpacket->flags == IMG_FLAG_ROTATE_180)
            flip_left_to_right(img_matrix, result_matrix, width, height);
        else if (recvpacket->flags == IMG_FLAG_ROTATE_270) 
            flip_upside_down(img_matrix, result_matrix, width, height);

        uint8_t* img_array = malloc(sizeof(uint8_t) * (width) * (height)); ///Hint malloc using sizeof(uint8_t) * width * height

        flatten_mat(result_matrix, img_array, width, height);

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

        // return the processed image data
        char msg[8]; // to store image data
        memset(msg, 0, 8);
        while (1) {
            int bytes_read = fread(msg, sizeof(char), 8, rot_fd);
            if (bytes_read == 0)
                break;
            else if (bytes_read < 0)
                perror("fread failed");

            int bytes_sent = 0;
            bytes_sent = send(socket, msg, bytes_read, 0);
            if(bytes_sent == -1) // send message to client and error check
                perror("send error");
            memset(msg, 0, 8); // clear buffer for next read

            // wait for client to send ack before continuing to send more data
            char received_data[PACKETSZ];
            memset(received_data, 0, PACKETSZ);
            if (recv(socket, received_data, PACKETSZ, 0) == -1)
                perror("recv error");
            
            packet_t *received_packet = NULL;
            received_packet = deserializeData(received_data);
            if (received_packet->operation == IMG_OP_NAK) // if not acknowledge, skip image
                failed = 1;
            
            free(received_packet);
            if (failed) {
                send(socket, serialized_error, PACKETSZ, 0);
                break;
            }
        }

        // send smaller package to let server know done sending imagedata
        char end[4] = "END";
        if (send(socket, &end, sizeof(char) * 4, 0) == -1)
            perror("send error");

        // close file pointers
        if (fclose(fd) != 0)
            perror("issue closing temp file");
        if (fclose(rot_fd) != 0)
            perror("issue closing temp rot file");

        // delete temp files for next image
        if (remove(filename) != 0)
            perror("Issue with deleting temp file");
        if (remove(rotated_file) != 0)
            perror("Issue with deleting temp rot file");
    }

    // close file pointers
    if (fclose(fd) != 0)
        perror("issue closing temp file");
    if (fclose(rot_fd) != 0)
        perror("issue closing temp rot file");
    
    // delete temp files for clean up
    if (remove(filename) != 0)
        perror("Issue with deleting temp file");
    if (remove(rotated_file) != 0)
        perror("Issue with deleting temp rot file");

    free(serialized_ack);
    free(serialized_error);

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
    ret = listen(listen_fd, MAX_CLIENTS);
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

        pthread_t curr_thread;
        // create a handling thread for the client
        if(pthread_create(&curr_thread, NULL, clientHandler, (void *)&conn_fd) != 0){
            close(conn_fd); 
            close(listen_fd);
            perror("Error creating worker thread");
        }
        pthread_detach(curr_thread);
    }
    
    return 0;
}
