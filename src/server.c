#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"
#include "parser.h"
#include "http.h"

#define BUFF_SIZE 1000

int ff_start_proxy(struct ff_config *config)
{
    init_openssl();

    printf("Creating socket...\n");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in address;
    char buffer[BUFF_SIZE];
    int recv_len;
    struct sockaddr_storage src_address;
    socklen_t src_address_length;
    char ip[INET_ADDRSTRLEN];

    if (sockfd == 0) {
        perror("failed to open socket");
        exit(EXIT_FAILURE);
    }

    

    inet_ntop(AF_INET, &config->ip_address, ip, sizeof(ip));

    printf("Binding to address %.16s:%d\n", ip, config->port);
    address.sin_family = AF_INET;
    address.sin_addr = config->ip_address;
    address.sin_port = htons(config->port);

    if (bind(sockfd, (struct sockaddr*)&address, sizeof(address))) {
        perror("failed to bind to address");
        exit(EXIT_FAILURE);
    }

    while ((recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&src_address, &src_address_length))) {
        if (recv_len == -1) {
            perror("failed to read from socket");
            exit(EXIT_FAILURE);
        }

        printf("Received %d bytes from socket\n", recv_len);
        write(STDOUT_FILENO, &buffer, recv_len);
    }

    return 0;
}