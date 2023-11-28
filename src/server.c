#include "server.h"

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024 

//Stephen: only have copied stuff from lab 12 onto here, so struct stuff might not match
void *clientHandler(void *socket) {

    // Receive packets from the client
    char recvdata[PACKETSZ];
    memset(recvdata, 0, PACKETSZ);
    ret = recv(conn_fd, recvdata, PACKETSZ, 0);
    if(ret == -1)
        perror("recv error");    

    // Determine the packet operatation and flags
    Packet *recvpacket = deserializeData(recvdata);

    // Receive the image data using the size

    // Process the image data based on the set of flags

    // Acknowledge the request and return the processed image data
    Packet packet;
    packet.operation = htons(PROTO_ACK);
    strcpy(packet.data, recvpacket->data);
    char *serializedData = serializePacket(&packet);
    ret = send(conn_fd, serializedData, PACKETSZ, 0); 
    if(ret == -1)
        perror("send error");
}

int main(int argc, char* argv[]) {

    // Creating socket file descriptor
    int listen_fd, conn_fd;
    // Bind the socket to the port

    /* added by Stephen, still need to work out order of stuff
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen to any of the network interface (INADDR_ANY)
    servaddr.sin_port = htons(PORT); // Port number
    */
    int ret = bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)); // bind address, port to socket
    if(ret == -1)
        perror("bind error");

    // Listen on the socket

    listen_fd = socket(AF_INET, SOCK_STREAM, 0); // create listening socket
    if(listen_fd == -1)
        perror("socket error");

    // Accept connections and create the client handling threads

    struct sockaddr_in clientaddr;
    socklen_t clientaddr_len = sizeof(clientaddr);
    conn_fd = accept(listen_fd, (struct sockaddr *) &clientaddr, &clientaddr_len); // accept a request from a client
    if(conn_fd == -1)
        perror("accept error");

    // Release any resources

    // close sockets
    close(conn_fd); 
    close(listen_fd);

    return 0;
}
