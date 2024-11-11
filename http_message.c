#include "http_message.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

bool is_complete_http_message(char* buffer) {
    printf("Buffer receieved: '%s'\n", buffer);

    if(strlen(buffer) < 4) {
        return false;
    }
    if (strncmp(buffer, "GET ", 4) != 0) {
        return false;
    }
    // if (strncmp(buffer + strlen(buffer) - 2, "\n\n", 2) != 0) {
    //     return false;
    // }
    if (strlen(buffer) >= 4 && strstr(buffer, "\r\n\r\n") != NULL) {
        return true;
    }

    return false;
}

void read_http_client_message(int client_sock,
    http_client_message_t** msg, http_read_result_t* result) {

        // printf("test\n");

        *msg = malloc(sizeof(http_client_message_t));
        char buffer[1024];
        strcpy(buffer, "");

        while(!is_complete_http_message(buffer)) {
            int bytes_read = read(client_sock, buffer+strlen(buffer), sizeof(buffer) - strlen(buffer));

            printf("Bytes read: %d\n", bytes_read);
            printf("Buffer content: '%s'\n", buffer);
            if(bytes_read == 0) {
                *result = CLOSED_CONNECTION;
                return;
            }
            if(bytes_read < 0) {
                *result = BAD_REQUEST;
                return;
            }
        }

        // buffer[strlen(buffer)] = '\0';
        // (*msg)->method = strdup(buffer + 4);
        // *result = MESSAGE;

        char *method = strtok(buffer, " ");
        char *path = strtok(NULL, " ");
        (*msg)->method = strdup(method);
        (*msg)->path = strdup(path);
        *result = MESSAGE;
    }

void http_client_message_free(http_client_message_t* msg) { free(msg); }

int respond_to_static(int sock_fd, http_client_message_t* http_msg, char* path) {
        char *image_name = path + 15;
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "static/images/%s", image_name);
        printf("Image file path: %s\n", file_path);

        int img_fd = open(file_path, O_RDONLY);
        if (img_fd < 0) {
            char *response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            write(sock_fd, response, strlen(response));
            return -1;
        }

        struct stat file_stat;
        if (fstat(img_fd, &file_stat) < 0) {
            close(img_fd);
            char *response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
            write(sock_fd, response, strlen(response));
            return -1;
        }

        char buffer[1024];
        int bytes_read;
        char *response_header = "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n\r\n";
        write(sock_fd, response_header, strlen(response_header));

        while ((bytes_read = read(img_fd, buffer, sizeof(buffer))) > 0) {
            write(sock_fd, buffer, bytes_read);
        }

        if (bytes_read < 0) {
            perror("read");
        }

        close(img_fd);
        return 0;
}


int parse_query_param(const char *query, const char *key, int *value) {
    char *key_start = strstr(query, key);
    if (key_start) {
        char *value_start = key_start + strlen(key) + 1;
        char *end = strchr(value_start, '&');
        if (!end) end = value_start + strlen(value_start);
        char val_str[16];
        strncpy(val_str, value_start, end - value_start);
        *value = atoi(val_str);
        return 1;
    }

    return 0;
}


int respond_to_calc(int sock_fd, http_client_message_t* http_msg, char* path) {
    char *query = strchr(path, '?');
    if (!query) {
        char *response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        write(sock_fd, response, strlen(response));
        return -1;
    }

    query++; // Skips the '?'
    int a = 0;
    int b = 0;
    int found_a = parse_query_param(query, "a", &a);
    int found_b = parse_query_param(query, "b", &b);

    if(!(found_a && found_b)) {
        char *response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        write(sock_fd, response, strlen(response));

        return -1;
    }

    int sum = a + b;
    char response[256];
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Sum: %d</h1></body></html>", sum);
    write(sock_fd, response, strlen(response));

    return 0;
}


int respond_to_stats(int sock_fd, http_client_message_t* http_msg, char* path) {
    
}
