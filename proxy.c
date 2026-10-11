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
#include <netdb.h>

//predefined port for now
#define PORT 8080

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

//caching stuff
typedef struct CacheNode {
    char url[256];              
    char *responseData;        
    int responseSize;           
    struct CacheNode *prev;     
    struct CacheNode *next;     
} CacheNode;

//cache manager
typedef struct {
    CacheNode *head;          
    CacheNode *tail;           
    int currentSize;            
    int maxSize;    
    pthread_mutex_t lock; //for your multithreading bs, shree           
} LRUCache;

LRUCache *proxyCache = NULL;

//Client args struct passed to worker threads
typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
}client_args_t;

//init
void initCache(int maxSizeBytes) {
    proxyCache = (LRUCache *)malloc(sizeof(LRUCache));
    proxyCache->head = NULL;
    proxyCache->tail = NULL;
    proxyCache->currentSize = 0;
    proxyCache->maxSize = maxSizeBytes;
    pthread_mutex_init(&proxyCache->lock, NULL);
    printf("Cache initialized with %d bytes capacity.\n", maxSizeBytes);
}


CacheNode* findInCache(const char* targetUrl) {
    pthread_mutex_lock(&proxyCache->lock);
    CacheNode *current = proxyCache->head;
    
    while (current != NULL) {
        if (strcmp(current->url, targetUrl) == 0) {
	    pthread_mutex_unlock(&proxyCache->lock);
            return current; //cache hit
        }
        current = current->next;
    }
    pthread_mutex_unlock(&proxyCache->lock);
    return NULL; //cache miss
}

//big boi cache function
void insertIntoCache(const char *targetUrl, const char *responseData, int responseSize) {
    pthread_mutex_lock(&proxyCache->lock);

    //delete oldest website if max limit reached
    while (proxyCache->currentSize + responseSize > proxyCache->maxSize && proxyCache->tail != NULL) {
        CacheNode *oldTail = proxyCache->tail;
        
        //disconnect tail from the list
        proxyCache->tail = oldTail->prev;
        if (proxyCache->tail != NULL) {
            proxyCache->tail->next = NULL;
        } else {
            proxyCache->head = NULL; //cache is now empty
        }
        
        //free the memory and update the size tracker
        proxyCache->currentSize -= oldTail->responseSize;
        free(oldTail->responseData);
        free(oldTail);
    }

    CacheNode *newNode = (CacheNode *)malloc(sizeof(CacheNode));
    strncpy(newNode->url, targetUrl, 255);
    newNode->url[255] = '\0';
    
    //big dynamic copy
    newNode->responseData = (char *)malloc(responseSize);
    memcpy(newNode->responseData, responseData, responseSize);
    newNode->responseSize = responseSize;

    //insert new node at head
    newNode->next = proxyCache->head;
    newNode->prev = NULL;

    if (proxyCache->head != NULL) {
        proxyCache->head->prev = newNode;
    }
    proxyCache->head = newNode;

    //first item = tail
    if (proxyCache->tail == NULL) {
        proxyCache->tail = newNode;
    }

    proxyCache->currentSize += responseSize;
    pthread_mutex_unlock(&proxyCache->lock);
}

//normal functions and stuff
void get_timestamp(char *buffer, size_t size) {
    time_t rawtime;
    struct tm timeinfo;

    time(&rawtime);
    localtime_r(&rawtime, &timeinfo); // Thread-safe on WSL/Linux

    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

void logMessage(const char *clientip, const char *method, const char *msg){
    pthread_mutex_lock(&log_mutex);
    FILE *log = fopen("proxy.log", "a");
    if(log != NULL){
	char timestamp[20];
	get_timestamp(timestamp, sizeof(timestamp));
	fprintf(log, "[%s] [%s] %s %s\n", timestamp, clientip, method, msg);
	fclose(log);
    }
    pthread_mutex_unlock(&log_mutex);
}

//i moved parsing from main to handle_client for multithreading (shree)
void *handle_client(void *arg){
    client_args_t *args = (client_args_t *)arg;
    int client_fd = args->client_fd;
    struct sockaddr_in client_addr = args->client_addr;

    free(args);
    pthread_detach(pthread_self());

    if(client_fd >=0){
        printf("Connection accepted from a client!\n");
        char client_ip[INET_ADDRSTRLEN];
	 inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
	 logMessage(client_ip, "CONNECT", "established");

         //Receive function
         char receive_buffer[4096] = {0};
         int readBytes = recv(client_fd, receive_buffer, sizeof(receive_buffer) - 1, 0);

         if (readBytes > 0){
              printf("Successfully received %d bytes\n", readBytes);
              printf("%s", receive_buffer);
          }

            //storing target domain name
            char target[256] = {0};
            char* targetPointer = strstr(receive_buffer, "Host: ");

            //if pointer finds the header, skip 6 to ignore "Host:" literal and copy the bling into the blong
            if (targetPointer != NULL){
                targetPointer += 6;
                sscanf(targetPointer, "%255[^\r\n]", target);
                printf("Target Extracted: %s\n", target);

                //Tokenize to separate the host and port
                char* hostname;
                char* port;
		char* saveptr;

                hostname = strtok_r(target, ":", &saveptr);
                port = strtok_r(NULL, ":", &saveptr); //calling strtok with NULL basiclaly means it'll grab the remaining half

                //in case user does not provide a port, fallback to HTTP default
                if (port == NULL){
                    port = "80";
                }

                printf("Parsing Complete.\nHostname: %s\nPort: %s\n", hostname, port);

                //now we translate plaintext hostname into an actual IP address
                struct addrinfo hints, *targetInfo;
                memset(&hints, 0, sizeof(hints));
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;

                int status = getaddrinfo(hostname, port, &hints, &targetInfo);

                if (status != 0){
                    printf("DNS ERROR\n");
                } else {
                    printf("DNS Resolved.\n");

                    int target_fd = socket(targetInfo->ai_family, targetInfo->ai_socktype, targetInfo->ai_protocol);
                    if (target_fd < 0){
                        printf("Target socket could not be created :(\n");
                    } else {
                        if (connect(target_fd, targetInfo->ai_addr, targetInfo->ai_addrlen) < 0){
                            printf("Connection to target failed.\n");
                        } else {
                            printf("Connection successful to target server!\n");

                            //forwarding logic
                            if (send(target_fd, receive_buffer, readBytes, 0) < 0){
                                printf("Forwarding request to target failed.\n");
                            } else {
                                printf("Request forwarded successfully. Awaiting response :D\n");

                                char targetResponse[4096];
                                int targetBytes;

                                while ((targetBytes = recv(target_fd, targetResponse, sizeof(targetResponse), 0)) > 0) {
                                    send(client_fd, targetResponse, targetBytes, 0);
                                }

                                printf("Response sent to client.\n");
                            }

                            close(target_fd);
                        }
                        freeaddrinfo(targetInfo);
                    }                       
                }

            } else {
                printf("ERROR: Host header not found! Contact admin or something..");
            }

            //close client only
            close(client_fd);
        }
return NULL;
}
	
int main() {
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    initCache(5242880);

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
    client_args_t *args = (client_args_t *)malloc(sizeof(client_args_t));
    socklen_t client_len = sizeof(args->client_addr);
    args->client_fd = accept(server_fd, (struct sockaddr*)&args->client_addr, &client_len);
    
    if(args->client_fd>=0){
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, handle_client, args);
    }
    else free(args);

    }
    close(server_fd);
    return 0;
}
