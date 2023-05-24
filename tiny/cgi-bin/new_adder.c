/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"
#define homework_11_11
//homework 11.10
int main(void) 
{
  char *buf, *p, *arg1_p, *arg2_p, *method;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE], val1[MAXLINE], val2[MAXLINE];
  int n1=0, n2=0;

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL) 
  {
    p = strchr(buf, '&');
    *p = '\0';
    sscanf(buf, "fnum=%d", &n1);
    sscanf(p+1, "snum=%d", &n2);
  }
  method = getenv("REQUEST_METHOD");
  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);
  
  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  
  // method가 GET일 경우에만 response body 보냄
  #ifdef homework_11_11
    if (strcasecmp(method, "GET") == 0) 
      printf("%s", content);
  #else
    printf("%s", content);
  #endif
  fflush(stdout);

  exit(0);
}

