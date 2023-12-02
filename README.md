**Project group number:** PA 27

**Group member names and x500s:**
1. Stephen Ma, ma000094
2. Robert Wang, wan00379
3. Robert Tan, tan00222

**The name of the CSELabs computer that you tested your code on:**
csel-kh1250-21.cselabs.umn.edu

**Members’ individual contributions Plan:**  
Stephen Ma:
* client.c: setting up sockets
* client.c: main
* server.c: clientHandler
* server.c: setting up sockets and accepting connections

Robert Wang:
* client.c: main
* server.c: main

Robert Tan:
* client.c: receive_file, send_file
* client.c: directory traversal
* server.c: clientHandler

**Plan on how you are going to construct the client handling thread and how you plan to send the package according to the protocol**  
In the main function of server.c, we will begin by creating a listening socket. We will bind it with the necessary addresses and proceed to call listen in order to listen for incoming client connections. The server should constantly be waiting for an incoming client connection, and it will terminate only when the user presses Ctrl + C. When it is able to accept a connection, it will create a client handling thread which will run the function clientHandler. This function will be in charge of receiving packets and sending packets to the client it is connected to through the socket. The packet it receives from the client will be used to determine the packet operation and flag. The image data will then be processed and sent back to the respective client in addition to the server response packet. On the client side, it should save the image data in a buffer and send it to the server (client handling thread). In addition, the client should also send a packet with the IMG_FLAG_ROTATE_XXX message header, desired rotation Angle, and Image size. Once that has been processed on the server side, the client should receive a response packet containing the processed image, and it will save the image to the output directory. 
