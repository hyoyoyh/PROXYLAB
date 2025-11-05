/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;
  if ((buf = getenv("QUERY_STRING")) != NULL) {
    p = strchr(buf, '&');
    if (p) {
      *p = '\0';
      strcpy(arg1, buf);
      strcpy(arg2, p+1);
      n1 = atoi(arg1);
      n2 = atoi(arg2);
    }
  }
  sprintf(content, "<html><head><title>더하기</title></head>\r\n");
  sprintf(content + strlen(content), "<p>The answer is: %d + %d = %d</p>\r\n", n1, n2, n1 + n2);

  printf("Connection: close\r\n");
  printf("Content-length: %zu\r\n", strlen(content));
  printf("Content-type: text/html\r\n\r\n");
 
  printf("%s", content);
  fflush(stdout); //printf로 쌓은 데이터 브라우저로 보내기 writen이랑 비슷한거가틈
  return 0;
}
/* $end adder */
