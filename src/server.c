#include "server.h"

#define PORT 4891
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024 

// global vars
pthread_t workerArray[BUFFER_SIZE];
int worker_idx = 0;
int workers_done = 0;

void *clientHandler(void *socket) {
    // Receive packets from the client
    char recvdata[PACKETSZ];
    memset(recvdata, 0, PACKETSZ);
    int ret = recv((int)socket, recvdata, PACKETSZ, 0);
    if(ret == -1)
        perror("recv error");    

    // Determine the packet operatation and flags
    packet_t *recvpacket = deserializeData(recvdata);

    while(recvpacket->operation != ntohl(IMG_OP_EXIT)){
        ret = recv((int)socket, recvdata, PACKETSZ, 0);
        if(ret == -1)
            perror("recv error");
            
        recvpacket = deserializeData(recvdata);    

        packet_t packet;
        packet.operation = htonl(IMG_OP_ACK);
        char *serializedData = serializePacket(&packet);
        ret = send((int)socket, serializedData, PACKETSZ, 0); 
        if(ret == -1)
            perror("send error");

    }

    workers_done++;
    // Receive the image data using the size

    // Process the image data based on the set of flags

    // Acknowledge the request and return the processed image data
    
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

    // Release any resources

    return 0;
}
