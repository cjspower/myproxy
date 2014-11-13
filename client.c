//
//  main.c
//  test
//
//  Created by AndersonCHEN on 10/27/14.
//  Copyright (c) 2014 AndersonCHEN. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <pthread.h>

int main(int argc, const char * argv[]) {
    
    int client_fd;
    char buf[10000];
    unsigned int port;
    struct sockaddr_in addr,remoteaddr;
    
    if (argc<5) {
        printf("Usage: client server port url filename\n");
        exit(1);
    }
    
    memset(&remoteaddr, 0, sizeof remoteaddr);
    memset(&addr, 0, sizeof addr);
    
    
    port = atoi(argv[2]);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    
    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_port = htons(port);
    if (inet_aton(argv[1], (struct in_addr *)&remoteaddr.sin_addr.s_addr) == 0) {
        perror(argv[1]);
        exit(errno);
    }
    
    if((client_fd = socket(AF_INET, SOCK_STREAM, 0))==-1){
        perror("Cannot create socket");
        exit(1);
    }

    if((connect(client_fd, (struct sockaddr*)(&remoteaddr), sizeof(struct sockaddr))==-1)){
        perror("connect:");
        exit(1);
    }
    
    char request[100];
    memset(request, 0, 100);
    
    sprintf(request, "GET %s HTTP/1.0\r\n\r\n", argv[3]);
    
    if ((send(client_fd, request, strlen(request), 0))==-1) {
        perror("send");
        exit(1);
    }
    
    FILE *fp;
    fp=fopen(argv[4], "w");
    int recvlen;
    char *flag;
    memset(buf, 0, 10000);
    if((recvlen = recv(client_fd, buf, 10000, 0))<=0){
        perror("recv");
        fclose(fp);
        close(client_fd);
        return 0;
    }
    
    if ((*(buf+9)=='4')&&(*(buf+10)=='0')&&(*(buf+11)=='4')) {
        printf("%s",buf);
        fclose(fp);
        close(client_fd);
        remove(argv[4]);
        return 0;
    }
        
    flag = strstr(buf, "\r\n\r\n");
    fwrite(flag+4, 1, strlen(flag)-4, fp);
    

    memset(buf, 0, 10000);
    while ((recvlen = recv(client_fd, buf, 10000, 0))>0) {
        fwrite(buf, 1, recvlen, fp);
        memset(buf, 0, 10000);
    }
    fclose(fp);
    close(client_fd);
    return 0;
}