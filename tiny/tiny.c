#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
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

void doit(int fd) {
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s", buf);

    sscanf(buf, "%s %s %s", method, uri, version);

    read_requesthdrs(&rio);

    is_static = parse_uri(uri, filename, cgiargs);

    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }

    if (is_static)
        serve_static(fd, filename, sbuf.st_size);
    else
        serve_dynamic(fd, filename, cgiargs);
}

void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        printf("%s", buf);
        Rio_readlineb(rp, buf, MAXLINE);
    }
    return;
}

int parse_uri(char *uri, char *filename, char *cgiargs) {
  if (strstr(uri, "cgi-bin")) {
    char *cp = strchr(uri,'?');
    if (cp) {
      strcpy(cgiargs, cp + 1);
      *cp = '\0';
    }
    else {
      strcpy(cgiargs, "");
    }
    strcpy(filename, "."); 
    strcat(filename, uri); // ./cgibin // cgi-bin/adder?1232313
    return 0;
  }
  else {
    strcpy(cgiargs, "");
    strcpy(filename, "."); 
    strcat(filename, uri); //.uri
    if (uri[strlen(uri)-1] == '/')
    strcat(filename, "home.html");

    return 1;
  }
}

void serve_static(int fd, char *filename, int filesize) {
    int srcfd;
    char *srcp;
    char filetype[MAXLINE], buf[MAXBUF];

    // 파일 타입 결정
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");

    // 헤더 작성
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf + strlen(buf), "Server: Tiny Web Server\r\n");
    sprintf(buf + strlen(buf), "Content-length: %d\r\n", filesize);
    sprintf(buf + strlen(buf), "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(fd, buf, strlen(buf));

    // 파일 열기
    srcfd = Open(filename, O_RDONLY, 0);
    if (srcfd < 0) {
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }

    // mmap 시도
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    if (srcp == MAP_FAILED) {
        clienterror(fd, filename, "500", "Internal Error", "Tiny couldn't map the file");
        Close(srcfd);
        return;
    }

    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}
void serve_dynamic(int fd, char *filename, char *cgiargs) {
  char buf[MAXBUF];
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) {
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
        char *emptylist[] = { filename , NULL};
        Execve(filename, emptylist, environ);
    }
    wait(NULL);
    }

// [1] 응답 헤더 전송
// [2] 프로세스 fork() → 자식/부모 분기
// [3] 자식: 환경 변수 설정 + 출력 리다이렉트 + Execve 실행
// [4] 부모: 자식 종료 대기
// [5] CGI 프로그램 결과가 브라우저로 전송

// O_RDONLY	Read Only	읽기 전용으로 파일 열기
// O_WRONLY	Write Only	쓰기 전용으로 열기
// O_RDWR	Read & Write	읽기/쓰기 모두 가능
// O_CREAT	Create	파일이 없으면 새로 생성
// O_TRUNC	Truncate	파일이 존재하면 내용 비움 (0바이트로 만듦)
// O_APPEND	Append	파일 끝에 추가 모드로 쓰기
// O_EXCL	Exclusive	O_CREAT와 함께 사용 시, 이미 존재하면 실패

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE] , body[MAXBUF];
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body + strlen(body), "<body bgcolor=\"ffffff\">\r\n");
  sprintf(body + strlen(body), "%s: %s\r\n", errnum, shortmsg);
  sprintf(body + strlen(body), "<p>%s: %s\r\n", longmsg, cause);
  sprintf(body + strlen(body), "<hr><em>The Tiny Web server</em>\r\n");

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd,body,strlen(body));
}
// [1] 오류 감지 (예: 파일 없음)
//      ↓
// [2] doit() 또는 serve_*() 내부에서 clienterror() 호출
//      ↓
// [3] HTML 바디 구성
//      ↓
// [4] HTTP 헤더 전송
//      ↓
// [5] HTML 본문(에러 메시지) 전송
//      ↓
// [6] 브라우저가 HTML 렌더링 → 에러 페이지 출력