#include "server.h"

#define PORT 4891
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024 

// global vars
pthread_t workerArray[BUFFER_SIZE];
int worker_idx = 0;
int workers_done = 0;

void *clientHandler(void *socket) {
    packet_t error_packet = {IMG_OP_NAK, "", htonl(0)};
    char *serialized_error = serializePacket(&error_packet);

    packet_t ack_packet = {IMG_OP_ACK, "", htonl(0)};
    char *serialized_ack = serializePacket(&ack_packet);

    char filename[BUFF_SIZE]; // unfinshed not filled out
    char rotated_file[BUFFER_SIZE]; // TODO!@U(C!@U(!@$()))

    FILE *fd = fopen(filename, "w");
    if (fd == NULL)
        perror("Error opening file\n");
    
    FILE *rot_fd = fopen(rotated_file, "w");
    if (rot_fd == NULL)
        perror("Error opening file\n");
    while (1) {
    
        // Receive packets from the client
        char recvdata[PACKETSZ];
        memset(recvdata, 0, PACKETSZ);
        int ret = recv((int)socket, recvdata, PACKETSZ, 0);
        if(ret == -1)
            send(socket, serialized_error, sizeof(packet_t), 0); 

        // Determine the packet operatation and flags, send acknowledgement
        packet_t *recvpacket = deserializeData(recvdata);
        int angle;
        if (recvpacket->operation == IMG_OP_EXIT)
            break;
        else if (recvpacket->operation == IMG_OP_ROTATE) {
            if (send(socket, serialized_ack, sizeof(packet_t), 0) == -1)
                send(socket, serialized_error, sizeof(packet_t), 0);

            if (recvpacket->flags == IMG_FLAG_ROTATE_180)
                angle = 180;
            else if (recvpacket->flags == IMG_FLAG_ROTATE_270)
                angle = 270;

        }

        // Receive the image data using the size
        long bytes_read = 0;
        char imgData[BUFF_SIZE + 1]; // +1 for null term
        memset(imgData, 0, BUFF_SIZE);
        while(bytes_read < recvpacket->size) {
            if (recv((int)socket, imgData, PACKETSZ, 0) == -1)
                send(socket, serialized_error, sizeof(packet_t), 0);
            fwrite(imgData, sizeof(char), BUFF_SIZE, fd);
            memset(imgData, 0, BUFF_SIZE);
        }

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

            /*
            linear_to_image takes: 
                The image_result matrix from stbi_load
                An image matrix
                Width and height that were passed into stbi_load
            
            */
            linear_to_image(image_result, img_matrix, width, height);
            
            ////TODO: you should be ready to call flip_left_to_right or flip_upside_down depends on the angle(Should just be 180 or 270)
            //both take image matrix from linear_to_image, and result_matrix to store data, and width and height.
            //Hint figure out which function you will call. 
            
            if (angle == 180)
                flip_left_to_right(img_matrix, result_matrix, width, height);
            else if (angle == 270) 
                flip_upside_down(img_matrix, result_matrix, width, height);

            uint8_t* img_array = malloc(sizeof(uint8_t) * (width) * (height)); ///Hint malloc using sizeof(uint8_t) * width * height

            ///TODO: you should be ready to call flatten_mat function, using result_matrix
            //img_arry and width and height; 
            flatten_mat(result_matrix, img_array, width, height);

            ///TODO: You should be ready to call stbi_write_png using:
            //New path to where you wanna save the file,
            //Width
            //height
            //img_array
            //width*CHANNEL_NUM

            // char path[BUFF_SIZE+2];
            // memset(path, 0, BUFF_SIZE+2);
            // sprintf(path, "%s/%s", rotated_file, get_filename_from_path());
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

        // Return the processed image data read from rotfd

    }

    if (fclose(fd) == -1)
        perror("issue closing temp file\n");
    if (fclose(rot_fd) == -1)
        perror("issue closing temp rot file\n");

    // delete temp files
    if (remove(filename) != 0)
        perror("Issue with deleting temp file\n");
    if (remove(rotated_file) != 0)
        perror("Issue with deleting temp rot file\n");
    
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
    bzero(&servaddr, sizeof(servaddr));
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
            perror("accept error");
        }

        // // INTER SUBMISSION
        // // Server receiving packet from client
        // char recvdata[PACKETSZ];
        // memset(recvdata, 0, PACKETSZ);
        // ret = recv(conn_fd, recvdata, PACKETSZ, 0); // receive data
        // if(ret == -1) {
        //     close(conn_fd); 
        //     close(listen_fd);
        //     perror("recv err");
        // }

        // FINAL SUBMISSION
        if(pthread_create(&workerArray[worker_idx], NULL, clientHandler, (void *)&conn_fd) != 0){
            fprintf(stderr, "Error creating worker thread.\n");
            return -1;
        }
        pthread_detach(workerArray[worker_idx]);
        worker_idx++;
    }

    return 0;
}
