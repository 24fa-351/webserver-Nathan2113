#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "http_message.h"

#define DEFAULT_PORT 80
#define LISTEN_BACKLOG 5

int respond_to_http_client_message(int sock_fd, http_client_message_t* http_msg) {
    char* response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    write(sock_fd, response, strlen(response));
    return 0;
}

// input pointer is freed by this function
void handleConnection(int* sock_fd_ptr) {
    int sock_fd = *sock_fd_ptr;
    free(sock_fd_ptr);

    while (1) {
        printf("Handling connection on %d\n", sock_fd);

        http_client_message_t* http_msg;
        http_read_result_t result;

        read_http_client_message(sock_fd, &http_msg, &result);
        if (result == BAD_REQUEST) {
            printf("Bad request\n");
            close(sock_fd);
            return;
        } else if (result == CLOSED_CONNECTION) {
            printf("Closed connection\n");
            close(sock_fd);
            return;
        }

        respond_to_http_client_message(sock_fd, http_msg);
        http_client_message_free(http_msg);
    }

    printf("done with connection %d\n", sock_fd);
}


int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in socket_address;
    memset(&socket_address, '\0', sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
    

    // if(strcmp(argv[1], "-p") == 0) {
    //     port = atoi(argv[2]);
    //     socket_address.sin_port = htons(port);
    // } else {
    //     socket_address.sin_port = htons(DEFAULT_PORT);
    // }

    if(argc > 1) {
        if(strcmp(argv[1], "-p") == 0) {
            port = atoi(argv[2]);
            socket_address.sin_port = htons(port);
        }
    } else {
        socket_address.sin_port = htons(DEFAULT_PORT);
    }

    

    int returnval;

    returnval = bind(
        socket_fd, (struct sockaddr*)&socket_address, sizeof(socket_address));
    if (returnval < 0) {
        perror("bind");
        return 1;
    }
    
    returnval = listen(socket_fd, LISTEN_BACKLOG);

    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    while (1) {
        pthread_t thread;
        int *client_fd_buf = malloc(sizeof(int));

        *client_fd_buf = accept(
            socket_fd, (struct sockaddr*)&client_address,
            &client_address_len);

        printf("accepted connection on %d\n",
        *client_fd_buf);

        pthread_create(&thread, NULL, (void* (*)
        (void*))handleConnection,
            (void*)client_fd_buf);
    }

    return 0;
}