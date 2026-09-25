//Includes
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

//predefined port for now
#define PORT 8080

int main() {
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        printf("Socket creation failed!\n");
        return 1;
    }

    printf("Socket creation successful!\n");

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    printf("Server bound to port: %d\n", PORT);

    //Make server listen passively w/ a backlog of 10 connections rn
    listen(server_fd, 10);
    printf("Server is listening for connections...\n");

    int client_fd;
    client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)sizeof(address));

    printf("Connection accepted from a client!\n");

    //close server and client.
    close(server_fd);
    close(client_fd);

    return 0;
}
