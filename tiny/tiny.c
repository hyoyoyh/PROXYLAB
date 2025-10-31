/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

void doit(int fd) {
  rio_t rio;
  int is_static;
  struct buf;  
  char buff[MAXLINE];
  char url[MAXLINE] , filename[MAXLINE] , cgiargs[MAXLINE] , method[MAXLINE] , version[MAXLINE];
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio,buff,MAXLINE);
  sscanf(buff, "%s %s %s", method,url,version);
  read_requesthdrs(&rio); // 헤더 함수
  is_static = parse_url(url,filename,cgiargs);
  if (stat(filename, buff) == -1) { //is empty == stat
    printf("%s", "파일 없음;;;;;;");
    return;
  }
  if (!is_static) {
    serve_dynamic(fd,filename,cgiargs);
  }
  else serve_static(fd,filename,cgiargs); 
  return;
}
//브라우저 → [doit()]
//       ① 요청라인 읽기 - 해결
//       ② 헤더 읽기 -
//       ③ URI 분석
//       ④ serve_static or serve_dynamic
//       ⑤ 응답 전송
//<메서드> <요청 대상 URI> <HTTP 버전>

void read_requesthdrs(rio_t *rp) {

}
// <메서드> <요청 대상 URI> <HTTP 버전>
// <헤더 이름>: <헤더 값>
// <헤더 이름>: <헤더 값>
// ...
// <빈 줄>
// <본문> (있을 수도 있음)