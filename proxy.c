//Includes
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

//predefined port for now
#define PORT 8080

void get_timestamp(char *buffer, size_t size) {
    time_t rawtime;
    struct tm timeinfo;

    time(&rawtime);
    localtime_r(&rawtime, &timeinfo); // Thread-safe on WSL/Linux

    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

void logMessage(const char *clientip, const char *method, const char *msg){
    FILE *log = fopen("proxy.log", "a");
    if(log != NULL){
	char timestamp[20];
	get_timestamp(timestamp, sizeof(timestamp));
	fprintf(log, "[%s] [%s] %s %s\n", timestamp, clientip, method, msg);
	fclose(log);
    }
}

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
    logMessage("0", "ONLINE", "Server is Online");
    
    while(1){
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
    	int client_fd;
    	client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    
    	if(client_fd >=0){
    	    printf("Connection accepted from a client!\n");
    	    char client_ip[INET_ADDRSTRLEN];
	    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
	    logMessage(client_ip, "CONNECT", "established");
        }
        //close server and client.
        close(client_fd);
	}
    close(server_fd);

    return 0;
}
