**Project group number:** PA 27

**Group member names and x500s:**
1. Stephen Ma, ma000094
2. Robert Wang, wan00379
3. Robert Tan, tan00222

**The name of the CSELabs computer that you tested your code on:**
csel-kh1250-21.cselabs.umn.edu

**Members’ individual contributions:**  
Stephen Ma:
* client.c: setting up sockets
* client.c: main
* server.c: clientHandler
* server.c: setting up sockets and accepting connections
* README's
* code checkup

Robert Wang:
* client.c: main
* server.c: main
* fixing memory issues in both client.c and server.c

Robert Tan:
* client.c: receive_file, send_file
* client.c: directory traversal
* client.c: main
* server.c: clientHandler
* server.c: main

**How you designed your program for Package sending and processing**  
client:
  main:
    create socket
    connect socket to server
    open directory
    while(directory has entry):
      enqueue png entry to queue
    while(1):
      call send_file
      call receive_file
    send IMG_OP_EXIT to client handling thread
    close socket
    
  send_file:
    open image file
    dequeue request
    populate packet fields with request information
    serialize data and send to clientHandler
    if IMG_OP_NAK was received:
      skip rest of code
    else if IMG_OP_ACK was received:
      read image data into buffer
      send to clientHandler
    
  receive_file:
    open file in output directory 
    receive processed image data from clientHandler 
    write the data to opened file
    
server:
  main:
    create sockets for listening and connecting
    bind listening socket 
    listen on the socket
    while(1):
      accept connection from client for connecting socket
      create handling thread that will call clientHandler
      detach thread
  
  clientHandler:
    while(1):
      receive packet from client
      deserialize packet
      if IMG_OP_EXIT is received:
        skip rest of code
      if received packet has IMG_OP_ROTATE operation:
        send acknowledgment packet to client
      receive image data and write into buffer
      if any errors are encountered:
        send IMG_OP_NAK and skip rest of job
      process image and send back processed image data
