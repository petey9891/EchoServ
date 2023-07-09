//
// Created by Andrew Peterson on 7/4/23.
//
// server.cpp -- a simple stream socket server
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

#define PORT "8080" // port clients will be connecting to
#define  BACKLOG 10 // pending connections

void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// get sockaddre, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct  sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct  sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, clientfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage client_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(nullptr, PORT, &hints, &servinfo)) != 0) {
        perror("ERROR getting address info");
        exit(EXIT_FAILURE);
    }

    // loop through all of the results and bind to the first we encounter
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("ERROR opening socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("ERROR setsockopt");
            exit(EXIT_FAILURE);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("ERROR unable to bind to address");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == nullptr) {
        perror("ERROR server failed to bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("ERROR unable to listen");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        perror("ERROR sigaction");
        exit(EXIT_FAILURE);
    }

    printf("Server: waiting for connections...\n");

    while (true) { // main accept loop
        sin_size = sizeof client_addr;
        clientfd = accept(sockfd, (struct sockaddr*) &client_addr, &sin_size);

        if (clientfd == -1) {
            perror("ERROR unable to accept new connection");
            continue;
        }

        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr*) &client_addr), s, sizeof s);
        printf("Server: got connection from %s\n", s);

        if (!fork()) { // child process
            close(sockfd); // child doesn't need the listener
            if (send(clientfd, "Hello, world!", 13, 0) == -1) {
                perror("ERROR unable to send");
            }
            close(clientfd);
            exit(EXIT_SUCCESS);
        }
        close(clientfd); // parent doesn't need this

    }

    return 0;
}