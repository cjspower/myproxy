//
//  proxy.c
//  homework3
//
//  Created by AndersonCHEN on 10/17/14.
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

#define PORT "80"

int compare_time(char *old, char *new);
pthread_mutex_t file_lock[10];

char *day[7]={
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

char *month[12]={
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

typedef struct cache_
{
    char url[256];
    char last_modified[50];
    char expiry[50];
    char filename[10];
    int full;
}cache;

cache cachetable[10];

int read_request(char *request, char *host, int *port,char *url, char *name){
    
    char method[256];
    char protocol[256];
    char *uri;
    int num,length,readnum=0;
    num = sscanf(request, "%s %s %s", method, url, protocol);
    if(strcmp(method, "GET")!=0) return -1;
    if(strcmp(protocol, "HTTP/1.0")!=0) return -1;
    
    uri=url;
    uri=uri+7;
    sscanf(uri, "%255[^/:]", host);
    length=strlen(host);
    if(uri[length] == ':'){
        sscanf(&uri[length+1], "%d%n", port, &readnum);
        readnum++;
    }
    else *port = 80;
    
    strcpy(name, &uri[length+readnum]);
    
    return num;
}

int check_cache(char *url){
    
    int i=0;
    for (i=0; i<10; i++) {
        if (strcmp(cachetable[i].url, url)==0) {
            return 0;
        }
    }
    return -1;
}

int monthcoverter(char *month){
    int m;
    switch (*month) {
        case 'J':
            if (*(month+1)=='a') {
                m=1;
            }
            else if (*(month+2)=='n'){
                m=6;
            }
            else m=7;
            break;
        case 'F':
            m=2;
            break;
        case 'M':
            if (*(month+2)=='r') {
                m=3;
            }
            else m=5;
            break;
        case 'A':
            if (*(month+2)=='r') {
                m=4;
            }
            else m=8;
            break;
        case 'S':
            m=9;
            break;
        case 'O':
            m=10;
            break;
        case 'N':
            m=11;
            break;
        case 'D':
            m=12;
            break;
        default:
            break;
    }
    return m;
}

int check_expire(char *url,struct tm *timenow, int *cachenum){
    
    int i=0,m;
    for (i=0; i<10; i++) {
        if (strcmp(cachetable[i].url, url)==0) {
            break;
        }
    }
    char newtime[50];
    memset(newtime, 0, 50);
    *cachenum = i;
    
    sprintf(newtime, "%s, %2d %s %4d %2d:%2d:%2d GMT", day[timenow->tm_wday],timenow->tm_mday, month[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
    
    m = compare_time(cachetable[i].expiry, newtime);
    printf("%d\n",m);
    if (m<0) {
        return -1;
    }
    else return 0;
}

int compare_time(char *old, char *new){
    int old_year,old_hour, old_minute, old_second,old_day;
    int new_year,new_hour, new_minute, new_second,new_day;
    
    int omn,nmn;
    
    char old_month[4];
    char new_month[4];
    
    memset(old_month, 0, 4);
    memset(new_month, 0, 4);
    
    sscanf(old+5, "%d %3s %d %d:%d:%d ",&old_day,old_month,&old_year,&old_hour,&old_minute,&old_second);
    sscanf(new+5, "%d %3s %d %d:%d:%d ",&new_day,new_month,&new_year,&new_hour,&new_minute,&new_second);
    omn = monthcoverter(old_month);
    nmn = monthcoverter(new_month);
    
    if (old_year<new_year) return -1;
    if (old_year>new_year) return 1;
    if (omn<nmn) return -1;
    if (omn>nmn) return 1;
    if (old_day<new_day) return -1;
    if (old_day>new_day) return 1;
    if (old_hour<new_hour) return -1;
    if (old_hour>new_hour) return 1;
    if (old_minute<new_minute) return -1;
    if (old_minute>new_minute) return 1;
    if (old_second<new_second) return -1;
    if (old_second>new_second) return 1;
    return 0;
}


void send_error(int status, char* text, int sock){
    char errbuf[1024];
    static char* bad_request =
    "HTTP/1.0 400 Bad Request\r\n"
    "Content-type: text/html\r\n"
    "\r\n"
    "<html>\r\n"
    " <body>\r\n"
    "  <h1>Bad Request</h1>\r\n"
    " <p>This server did not understand your request.</p>\r\n"
    " </body>\r\n"
    "</html>\r\n";
    
    static char* not_found =
    "HTTP/1.0 404 Not Found\r\n"
    "Content-type: text/html\r\n"
    "\r\n"
    "<html>\r\n"
    " <body>\r\n"
    " <h1>Not Found</h1>\r\n"
    " <p>The requested URL %s was not found on this server.</p>\r\n"
    " </body>\r\n"
    "</html>\r\n";
    
    memset(errbuf, 0, 1024);
    
    
    switch (status) {
        case 400:
            sprintf(errbuf, "%s", bad_request);
            send(sock, errbuf, strlen(errbuf), 0);
            break;
            
        case 404:
            sprintf(errbuf, "%s", not_found);
            send(sock, errbuf, strlen(errbuf), 0);
            break;
            
        default:
            break;
    }
}


int processing(int client_fd){
    struct addrinfo hints, *answer;
    int port=80;
    int proxyfd;
    char buf[10000], host[256], url[256], name[256];
    
    memset(buf, 0, 10000);
    memset(host, 0, 256);
    memset(url, 0, 256);
    memset(name, 0, 256);
    if(recv(client_fd, buf, sizeof(buf), 0)==-1){
        perror("recv");
        close(client_fd);
        return 1;
    }
    
    
    if (read_request(buf, host, &port, url, name)!=3) {
        send_error( 400, "Can not analyse request",client_fd);
        close(client_fd);
        return 1;
    }
    
    printf("Request\nMethod:%s\nHost:%s:%d\nFile:%s\n","GET",host,port,name);
    
    struct sockaddr_in proxyaddr;
    
    memset(&proxyaddr, 0, sizeof proxyaddr);
    
    proxyaddr.sin_addr.s_addr = INADDR_ANY;
    proxyaddr.sin_family = AF_INET;
    
    if((proxyfd = socket(AF_INET, SOCK_STREAM, 0))==-1){
        perror("proxysocket create");
        close(client_fd);
        return 1;
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if(getaddrinfo(host, PORT, &hints, &answer)!=0){
        printf("getaddrinfo error\n");
        send_error( 404, "resolution error",client_fd);
        close(client_fd);
        return 1;
    }
    
    if (connect(proxyfd, answer->ai_addr, answer->ai_addrlen)==-1) {
        close(proxyfd);
        perror("connect");
        send_error( 404, "connect error",client_fd);
        close(client_fd);
        return 1;
    }
    
    time_t now;
    struct tm *timenow;
    time(&now);
    timenow = gmtime(&now);
    int recvlen = -1;
    if(check_cache(url)==-1){
        char request[1024];
        int sendlen;
        memset(request, 0, 1024);
        printf("file not in cache, send request to server.\n");
        sprintf(request, "GET http://%s:%d%s HTTP/1.0\r\n\r\n",host,port,name);
        puts(request);
        if((sendlen=send(proxyfd, request, strlen(request), 0))==-1){
            perror("send request:");
            close(proxyfd);
            return 1;
        }
        printf("rquest sent length: %d\n",sendlen);
        memset(buf, 0, 10000);
        int i = 0,oldest = 0;
        for (i=0; i<10; i++) {
            if(cachetable[i].full==0) {
                oldest = i;
                break;
            }
            else{
                if (compare_time(cachetable[i].last_modified,cachetable[oldest].last_modified)<=0) {
                    oldest = i;
                }
            }
        }
        
        if (pthread_mutex_trylock(&file_lock[oldest])==0) {
            
            memset(&cachetable[oldest], 0, sizeof(cache));
            cachetable[oldest].full=1;
            sprintf(cachetable[oldest].filename,"%d",oldest);
            memcpy(cachetable[oldest].url,url,256);
            sprintf(cachetable[oldest].last_modified, "%s, %02d %s %d %02d:%02d:%02d GMT", day[timenow->tm_wday],timenow->tm_mday, month[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
            
            FILE *fp;
            FILE *readfp;
            remove(cachetable[oldest].filename);
            fp=fopen(cachetable[oldest].filename, "w");
            if (fp==NULL) {
                printf("failed to create cache.\n");
                pthread_mutex_unlock(&file_lock[oldest]);
                return 1;
            }
            puts(cachetable[oldest].last_modified);
            //printf("gethere1\n");
            while ((recvlen=recv(proxyfd, buf, 10000, 0))>0) {
                
                //printf("gethere2\n");
                if(send(client_fd, buf, recvlen, 0)==-1){
                    perror("Send to client:");
                    pthread_mutex_unlock(&file_lock[oldest]);
                    return 1;
                }
                fwrite(buf, 1, recvlen, fp);
                memset(buf, 0, 10000);
            }
            fclose(fp);
            readfp=fopen(cachetable[oldest].filename,"r");
            
            fread(buf, 1, 2048, readfp);
            fclose(readfp);
            char* expires=NULL;
            
            expires=strstr(buf, "Expires: ");
            if (expires!=NULL) {
                memcpy(cachetable[oldest].expiry, expires+9, 29);
            }
            else{
                sprintf(cachetable[oldest].expiry, "%s, %02d %s %4d %02d:%02d:%02d GMT", day[timenow->tm_wday],timenow->tm_mday, month[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
            }
            pthread_mutex_unlock(&file_lock[oldest]);
        }
        else{
            printf("Another thread is trying to operate on the same cache file, run as repeater.\n");
            while ((recvlen=recv(proxyfd, buf, 10000, 0))>0) {
                
                if(send(client_fd, buf, recvlen, 0)==-1){
                    perror("Send to client:");
                    return 1;
                }
                memset(buf, 0, 10000);
            }
        }
        
    }
    else{
        int cachenum;
        int readlen = 0;
        printf("detect file in cache, check expire.\n");
        if(check_expire(url,timenow,&cachenum)>=0){
            printf("file not expired, send to client\n");
            sprintf(cachetable[cachenum].last_modified, "%s, %02d %s %4d %02d:%02d:%02d GMT", day[timenow->tm_wday],timenow->tm_mday, month[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
            FILE *readfp;
            readfp=fopen(cachetable[cachenum].filename,"r");
            memset(buf, 0, 10000);
            while ((readlen=fread(buf, 1, 10000, readfp))>0) {
                send(client_fd, buf, readlen, 0);
            }
            fclose(readfp);
        }
        else{
            printf("file expired, send request to server");
            int recvlen = 0;
            char modified[100];
            memset(modified, 0, 100);
            sprintf(modified, "If-Modified-Since: %s\r\n\r\n",cachetable[cachenum].last_modified);
            //sprintf(modified, "If-Modified-Since: %s, %2d %s %4d %2d:%2d:%2d GMT\r\n\r\n", day[timenow->tm_wday],timenow->tm_mday, month[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
            char modified_request[10000];
            memset(modified_request, 0, 10000);
            memcpy(modified_request, buf, strlen(buf)-2);
            
            strcat(modified_request, modified);
            printf("%s\n",modified_request);
            send(proxyfd, modified_request, strlen(modified_request), 0);
            memset(buf, 0, 10000);
            recvlen = recv(proxyfd, buf, 10000, 0);
            char *expires=NULL;
            expires=strstr(buf, "Expires: ");
            if (expires!=NULL) {
                memcpy(cachetable[cachenum].expiry, expires+9, 29);
            }
            else{
                sprintf(cachetable[cachenum].expiry, "%s, %02d %s %4d %02d:%02d:%02d GMT", day[timenow->tm_wday],timenow->tm_mday, month[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
            }
            
            if (recvlen>0) {
                if ((*(buf+9)=='3')&&(*(buf+10)=='0')&&(*(buf+11)=='4')) {
                    printf("%s",buf);
                    printf("304 Not Modified, Send file in cache\n");
                    
                    FILE *readfp;
                    readfp=fopen(cachetable[cachenum].filename,"r");
                    memset(buf, 0, 10000);
                    while ((readlen=fread(buf, 1, 10000, readfp))>0) {
                        send(client_fd, buf, readlen, 0);
                    }
                    fclose(readfp);
                    
                }
                else if((*(buf+9)=='4')&&(*(buf+10)=='0')&&(*(buf+11)=='4')){
                    send_error( 404, "Not found",client_fd);
                }
                else if((*(buf+9)=='2')&&(*(buf+10)=='0')&&(*(buf+11)=='0')){
                    printf("200 OK, new file get, send new file and refresh cache.\n");
                    send(client_fd, buf, recvlen, 0);
                    if (pthread_mutex_trylock(&file_lock[cachenum])==0) {
                        remove(cachetable[cachenum].filename);
                        
                        char* expires=NULL;
                        
                        expires=strstr(buf, "Expires: ");
                        if (expires!=NULL) {
                            memcpy(cachetable[cachenum].expiry, expires+9, 29);
                        }
                        else{
                            sprintf(cachetable[cachenum].expiry, "%s, %02d %s %4d %02d:%02d:%02d GMT", day[timenow->tm_wday],timenow->tm_mday, month[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
                        }
                        
                        sprintf(cachetable[cachenum].last_modified, "%s, %02d %s %4d %02d:%02d:%02d GMT", day[timenow->tm_wday],timenow->tm_mday, month[timenow->tm_mon], timenow->tm_year+1900,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
                        FILE *fp;
                        fp=fopen(cachetable[cachenum].filename, "w");
                        fwrite(buf, 1, recvlen, fp);
                        
                        memset(buf, 0, 10000);
                        while ((recvlen=recv(proxyfd, buf, 10000, 0))>0) {
                            send(client_fd, buf, recvlen, 0);
                            fwrite(buf, 1, recvlen, fp);
                        }
                        fclose(fp);
                        pthread_mutex_unlock(&file_lock[cachenum]);
                    }
                    else{
                        printf("Another thread is trying to operate on the same cache file, run as repeater.\n");
                        memset(buf, 0, 10000);
                        while ((recvlen=recv(proxyfd, buf, 10000, 0))>0) {
                            send(client_fd, buf, recvlen, 0);
                        }
                    }
                }
            }
            else perror("receive:");
        }
        
    }
    
    
    close(client_fd);
    close(proxyfd);
    return 0;
    
}

int main(int argc, const char * argv[]) {
    
    pthread_t thread_id;
    int port,listener,client_fd;
    struct sockaddr_storage remoteaddr;
    struct sockaddr_in addr;
    
    if (argc<3) {
        printf("\nUsage: server host port\nExiting\n");
        exit(1);
    }
    memset(&remoteaddr, 0, sizeof remoteaddr);
    memset(&addr, 0, sizeof addr);
    
    
    port = atoi(argv[2]);
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_family = AF_INET;
    
    if((listener = socket(AF_INET, SOCK_STREAM, 0))==-1){
        perror("Cannot create socket");
        exit(1);
    }
    if(bind(listener, (struct sockaddr*)&addr, sizeof(struct sockaddr))==-1) {
        perror("Cannot bind socket");
        exit(1);
    }
    if(listen(listener, 10) == -1){
        perror("listen");
        exit(1);
    }
    
    for (int j=0; j<10; j++) {
        pthread_mutex_init(&file_lock[j], NULL);
    }
    
    memset(cachetable, 0, 10*sizeof(cache));
    
    printf("Waiting for request!\n");
    socklen_t sin_size;
    sin_size = sizeof(remoteaddr);
    while (1) {
        client_fd = accept(listener, (struct sockaddr *)&remoteaddr, &sin_size);
        printf("accepted\n");
        if (listener == -1) {
            perror("accept");
            continue;
        }
        pthread_create(&thread_id, NULL, (void *)(&processing), (void *)client_fd);
    }
    
    return 0;
}