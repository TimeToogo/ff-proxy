#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "server.h"
#include "parser.h"

int main(int argc, char **argv)
{
    printf("Creating socket...\n");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in address;
    char buffer[BUFF_SIZE];
    int recv_len;
    struct sockaddr_storage src_address;
    socklen_t src_address_length;

    if (sockfd == 0) {
        perror("failed to open socket");
        exit(EXIT_FAILURE);
    }

    printf("Binding to address 0.0.0.0:%d\n", PORT);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

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