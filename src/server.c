#include "server.h"

#define PORT 4891
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024 

pthread_t workerArray[1024];
int worker_idx = 0;
int workers_done = 0;
//Stephen: only have copied stuff from lab 12 onto here, so struct stuff might not match
void *clientHandler(void *socket) {

    // Receive packets from the client

    char recvdata[PACKETSZ];
    memset(recvdata, 0, PACKETSZ);
    ret = recv(conn_fd, recvdata, PACKETSZ, 0);
    if(ret == -1)
        perror("recv error");    

    // Determine the packet operatation and flags
    packet_t *recvpacket = deserializeData(recvdata);

    while(recvpacket.operation != ntohl(IMG_OP_EXIT)){
        ret = recv(conn_fd, recvdata, PACKETSZ, 0);
        if(ret == -1)
            perror("recv error");
            
        recvpacket = deserializeData(recvdata);    

        packet_t packet;
        packet.operation = htons(PROTO_ACK);
        strcpy(packet.data, recvpacket->data);
        char *serializedData = serializePacket(&packet);
        ret = send(conn_fd, serializedData, PACKETSZ, 0); 
        if(ret == -1)
            perror("send error");

    }

    workers_done ++;
    // Receive the image data using the size

    // Process the image data based on the set of flags

    // Acknowledge the request and return the processed image data
    
}

int main(int argc, char* argv[]) {

    // Creating socket file descriptor
    int listen_fd, conn_fd;
    // Bind the socket to the port
    listen_fd = socket(AF_INET, SOCK_STREAM, 0); // create listening socket
    if(listen_fd == -1)
        perror("socket error");

    // added by Stephen, still need to work out order of stuff
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen to any of the network interface (INADDR_ANY)
    servaddr.sin_port = htons(PORT); // Port number

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
    while (1){

        // if(workers_done != 0 && workers_done == worker_idx){
        //     exit();
        // }
        struct sockaddr_in clientaddr;
        socklen_t clientaddr_len = sizeof(clientaddr);
        conn_fd = accept(listen_fd, (struct sockaddr *) &clientaddr, &clientaddr_len); // accept a request from a client

        if(conn_fd == -1)
            perror("accept error");

        // if(pthread_create(&workerArray[worker_idx], NULL, clientHandler, (void *)&conn_fd) != 0){
        //     fprintf(stderr, "Error creating worker thread.\n");
        //     return -1;
        // }
        // pthread_detach(workerArray[idx]);
        
        // worker_idx++;

        // INTER SUBMISSION
        // Server receiving packet from client
        char recvdata[PACKETSZ];
        memset(recvdata, 0, PACKETSZ);
        ret = recv(conn_fd, recvdata, PACKETSZ, 0); // receive data
        if(ret == -1)
            perror("recv err");
        
    }
    

    // Release any resources
    // for(int i = 0; i < worker_idx; i++){
    //     if(pthread_join(workerArray[i], NULL) != 0){
    //         fprintf(stderr, "Error joining worker thread.\n");
    //         return -1;
    //     }
    // }

    // close sockets
    // close(conn_fd); 
    // close(listen_fd);

    return 0;
}
