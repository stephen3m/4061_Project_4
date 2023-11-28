#include "client.h"

#define PORT 8080
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

    // Send the image data to the server

    // Check that the request was acknowledged

    // Receive the processed image and save it in the output dir

    // Terminate the connection once all images have been processed
    close(sockfd); // close socket

    // Release any resources
    
    return 0;
}
