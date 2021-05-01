//
//  main.c
//  project1_2015038477_Lee_Jaeha
//
//  Created by Jae ha Lee on 2021/04/12.
//


/*
   A simple server in the internet domain using TCP
   Usage:./server port (E.g. ./server 10000 )
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#define BUF_SIZE 1000
#define HEADER_FMT "HTTP/1.1 %d %s\nContent-Length: %ld\nContent-Type: %s\n\n"


void error(char *msg)
{
    perror(msg);
    exit(1);
}

/* format HTTP header by given parameter의 response status code */

void fill_header(char *header, int status, long len, char *type) {
    char status_text[40];
    if (status == 200){
        strcpy(status_text, "OK");
    }
    sprintf(header, HEADER_FMT, status, status_text, len, type);
}

/* uri로부터 find content type 받기*/

void find_mime(char *ct_type, char *uri) {
    char *ext = strrchr(uri, '.');
    if (!strcmp(ext, ".jpg") || !strcmp(ext, ".jpeg"))
        strcpy(ct_type, "image/jpeg");
    else if (!strcmp(ext, ".png"))
        strcpy(ct_type, "image/png");
    
}

/*TCP Socket*/

int bind_socket(int sockfd, int portno) {
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(portno);
    return bind(sockfd, (struct sockaddr *)&sin, sizeof(sin));
    
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd;
    int portno, pid;
    struct sockaddr_in _addr; socklen_t _addr_size;
     
    char buffer[BUF_SIZE];
    char header[BUF_SIZE];
     
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    
    portno = atoi(argv[1]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    /*소켓 생성 및 연결 에러 검사*/
    
    if (sockfd < 0)
        error("ERROR opening socket");
    
    if (bind_socket(sockfd, portno) < 0) {
        perror("ERROR failed to bind lsock.\n");
        exit(1);
        
    }
     
    if (listen(sockfd,5) < 0){
        perror("ERROR failed to listen lsock.\n");
        exit(1);
        
    }
    
    /*프로세스 생성*/
    signal(SIGCHLD, SIG_IGN); // 좀비 프로세스 방지
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &_addr, &_addr_size);
        if (newsockfd < 0) { perror("ERROR on accept\n");
            continue;
        }
        
        pid = fork();
        if (pid == 0) {
            close(sockfd);
            
            /* @func main http handler;
             requested된 파일들을 open과 send
             error가 발생할 시엔 error를 처리*/
            
            bzero(buffer, BUF_SIZE);
            long n = read(newsockfd,buffer,BUF_SIZE);
            if (n < 0)
                error("ERROR reading from socket");
            
            printf("\nHere is the message:\n%s\n",buffer);
            
            char *method = strtok(buffer, " ");
            char *uri = strtok(NULL, " ");
            if (method == NULL || uri == NULL) {
                error("ERROR Failed to identify method, URI.\n");
            }
            
            char safe_uri[BUF_SIZE];
            char *local_uri;
            struct stat st;
            strcpy(safe_uri, uri);
            
            if (!strcmp(safe_uri, "/"))
                strcpy(safe_uri, "/index.html");
            local_uri = safe_uri + 1;
            
            if (stat(local_uri, &st) < 0) {
                error("WARNING No file found matching URI.\n");
            }
                        
            int fd = open(local_uri, O_RDONLY);
            if (fd < 0) {
                error("ERROR failed to open file.\n");
            }
            
            long ct_len = st.st_size;
            char ct_type[40];
            find_mime(ct_type, local_uri);
            fill_header(header, 200, ct_len, ct_type);
            write(newsockfd, header, strlen(header));
            long cnt;
            while ((cnt = read(fd, buffer, BUF_SIZE)) > 0)
                write(newsockfd, buffer, cnt);

            close(newsockfd);
            exit(0);
            
        }
        if (pid != 0) {
            close(newsockfd);
            
        }
        
        if (pid < 0) {
            perror("failed to fork.\n");
        }
        
    }
    return 0;
}
