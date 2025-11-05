/* $begin proxy.c */
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) "
    "Gecko/20120305 Firefox/10.0.3\r\n";


void doit(int fd);
void read_requesthdrs(rio_t *rp);
void parse_uri(char *uri, char *hostname, char *path, int *port);
void build_http_header(char *http_header, char *hostname, char *path, rio_t *client_rio);
int connect_end_server(char *hostname, int port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    char hostname[MAXLINE], port[MAXLINE];
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
}

void doit(int fd)
{
    int end_server_fd;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], path[MAXLINE];
    int port;
    rio_t rio, server_rio;
    char http_header[MAXLINE];

    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))
        return;

    printf("Request headers:\n%s", buf);

    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "Proxy does not implement this method");
        return;
    }


    parse_uri(uri, hostname, path, &port);

    build_http_header(http_header, hostname, path, &rio);


    end_server_fd = connect_end_server(hostname, port);
    if (end_server_fd < 0) {
        printf("connection failed\n");
        return;
    }

    Rio_writen(end_server_fd, http_header, strlen(http_header));

    Rio_readinitb(&server_rio, end_server_fd);
    ssize_t n;
    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0) {
        printf("Proxy received %zd bytes, forwarding to client\n", n);
        Rio_writen(fd, buf, n);
    }

    Close(end_server_fd);
}

void build_http_header(char *http_header, char *hostname, char *path, rio_t *client_rio)
{
    char buf[MAXLINE], request_hdr[MAXLINE], other_hdr[MAXLINE], host_hdr[MAXLINE];
    sprintf(request_hdr, "GET %s HTTP/1.0\r\n", path);

 
    sprintf(host_hdr, "Host: %s\r\n", hostname);

    sprintf(other_hdr,
            "Connection: close\r\n"
            "Proxy-Connection: close\r\n"
            "%s", user_agent_hdr);

    while (Rio_readlineb(client_rio, buf, MAXLINE) > 0) {
        if (!strcmp(buf, "\r\n")) break;
        if (strncasecmp(buf, "Host:", 5) &&
            strncasecmp(buf, "Connection:", 11) &&
            strncasecmp(buf, "Proxy-Connection:", 17) &&
            strncasecmp(buf, "User-Agent:", 11)) {
            strcat(other_hdr, buf);
        }
    }

    sprintf(http_header, "%s%s%s\r\n", request_hdr, host_hdr, other_hdr);
}

void parse_uri(char *uri, char *hostname, char *path, int *port)
{
    *port = 80;
    strcpy(path, "/");

    char *hostbegin, *hostend, *portpos;

    if ((hostbegin = strstr(uri, "://")) != NULL)
        hostbegin += 3;
    else
        hostbegin = uri;

    hostend = strchr(hostbegin, '/');
    if (hostend != NULL) {
        strcpy(path, hostend);
        *hostend = '\0';
    }

    portpos = strchr(hostbegin, ':');
    if (portpos != NULL) {
        *portpos = '\0';
        *port = atoi(portpos + 1);
    }

    strcpy(hostname, hostbegin);
}

int connect_end_server(char *hostname, int port)
{
    char port_str[100];
    sprintf(port_str, "%d", port);
    return Open_clientfd(hostname, port_str);
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        printf("%s", buf);
        Rio_readlineb(rp, buf, MAXLINE);
    }
    return;
}

void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor="
                  "ffffff"
                  ">\r\n",
            body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy Web server</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

