//
// Created by Andrew Peterson on 7/4/23.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "8080" // the port that the client will be connecting to
#define MAXDATASIZE 256 // max number of bytes we can get at once

// get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct  sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct  sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buffer[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0) {
        perror("ERROR unable to get address info");
        exit(EXIT_FAILURE);
    }

    // look through all the results and connect to the first we can
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("ERROR unable to create the socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("ERROR unable to connect to server");
            continue;
        }

        break;
    }

    if (p == nullptr) {
        perror("ERROR failed to connect to server");
        exit(EXIT_FAILURE);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*) &p->ai_addr), s, sizeof s);
    printf("Client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    if ((numbytes = read(sockfd, buffer, sizeof(buffer) - 1)) == -1) {
        perror("ERROR unable to read data");
        exit(EXIT_FAILURE);
    }

    buffer[numbytes] = '\0';

    printf("Client: received '%s'\n", buffer);

    close(sockfd);

    return 0;
}