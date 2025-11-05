#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
void parse_uri(char *uri, char *hostname, char *path, int *port);
void build_http_header(char *http_header, char *hostname, char *path, rio_t *rio);
void read_requesthdrs(rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void connect_end_server(char *hostname, int *port);
int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char hostname[MAXLINE], port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen,
                    hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);

        doit(connfd);
        Close(connfd);
    }
}
void doit(int fd) {
    rio_t server_rio;
    rio_t rio;
    char buff[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], path[MAXLINE];
    int port;
    char http_header[MAXLINE];

    rio_readinitb(&rio,fd);
    rio_readlineb(&rio,buff,MAXLINE);
    sscanf(buff,"%s %s %s",method,uri,version);
    parse_uri(uri,hostname,path,&port);
    build_http_header(http_header,hostname,path,&rio);

    int serverfd;
    char port_str[10];
    sprintf(port_str, "%d", port);
    serverfd = Open_clientfd(hostname, port_str);
    if (serverfd < 0) {
        printf("connection failed\n");
        return;
    }

    Rio_writen(serverfd,http_header,strlen(http_header));
    rio_readinitb(&server_rio, serverfd);
    int n;
    while ((n = Rio_readlineb(&server_rio, buff, MAXLINE)) != 0) {
        Rio_writen(fd, buff, n);
    }

    Close(serverfd);
}

void read_requesthdrs(rio_t *rp) {
    char buff[MAXLINE];
    rio_readlineb(rp, buff, MAXLINE);
    while (strcmp(buff,"\r\n"))
    {
        printf("%s", buff);
        rio_readlineb(rp, buff, MAXLINE); // 내부에 버퍼 포인터가있어서 전에 읽었던 위치를 기억한다네요
    }
    
}
void parse_uri(char *uri, char *hostname, char *path, int *port) {
    *port = 80;           // 기본 포트
    strcpy(path, "/");    // 기본 경로

    char *hostbegin, *hostend, *portbegin;

    // "http://" 제거
    if ((hostbegin = strstr(uri, "://")) != NULL)
        hostbegin += 3;
    else
        hostbegin = uri;

    // path 찾기
    hostend = strchr(hostbegin, '/');
    if (hostend != NULL) {
        strcpy(path, hostend);
        *hostend = '\0';
    }

    // 포트 찾기
    portbegin = strchr(hostbegin, ':');
    if (portbegin != NULL) {
        *portbegin = '\0';
        *port = atoi(portbegin + 1);
    }

    strcpy(hostname, hostbegin);
}






void build_http_header(char *http_header, char *hostname, char *path, rio_t *rio) {
    char buff[MAXLINE];
    http_header[0] = '\0';
    sprintf(http_header, "GET /%s HTTP/1.0\r\n",path);
    sprintf(http_header + strlen(http_header), "Host: %s\r\n" , hostname);
    sprintf(http_header + strlen(http_header), "Connection: close\r\n");
    sprintf(http_header + strlen(http_header), "Proxy-Connection: close\r\n");
    sprintf(http_header + strlen(http_header), "User-Agent: %s\r\n",user_agent_hdr);
    rio_readlineb(rio,buff,MAXLINE);
    while (strcmp(buff,"\r\n"))
    {
        if (strncasecmp(buff, "Host", 4)
        && strncasecmp(buff, "User-Agent", 10)
        && strncasecmp(buff, "Connection", 10)
        && strncasecmp(buff, "Proxy-Connection", 16)) {
    strcat(http_header, buff);   }
    rio_readlineb(rio,buff,MAXLINE);
    }
    strcat(http_header, "\r\n");

}
