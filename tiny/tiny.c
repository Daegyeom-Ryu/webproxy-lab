/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
#define homework_11_7
#define homework_11_9
#define homework_11_10
#define homework_11_11

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char* method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);


int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);                 // stderr은 모니터 스트림, 바로 출력 
    exit(1);                                                        // 버퍼없이 바로 출력하므로 어떤 상황에서도 출력 가능 
  }
                                                                    // main 함수에서 받은 포트 번호를 통해 listenfd 식별자 생성
  listenfd = Open_listenfd(argv[1]);                                // listenfd는 계속 열려있고, 연결 요청이 들어올 때마다 연결 식별자인 connfd를 통해 파일을 읽고 쓴다.
  while (1) {
    clientlen = sizeof(clientaddr);
                                                                    // accept 함수는 클라이언트로부터 연결 요청(서버 host, 서버 port)이 listendfd 식별자에 도달하는 것을 기다림 
    connfd = Accept(listenfd, (SA *)&clientaddr,&clientlen);        // 연결되었다면 (클라이언트주소:포트, 서버주소:포트)의 유일한 튜플이 만들어짐 
    Getnameinfo((SA *)&clientaddr, clientlen, hostname,              
                MAXLINE, port, MAXLINE, 0);                         // 클라이언트 주소로부터 hostname, port 받아옴
    printf("Accepted connection from (%s, %s)\n", hostname, port);  
    doit(connfd);                                                   // 사용자 요청을 받아 doit 함수에서 처리한다. tiny.c 서버는 GET method만 가능       
    Close(connfd);                                                  // 요청에 대한 응답을 하고 나면 socket을 닫는다.
  }
}
/* doit에서 쓰이는 stat struct */
// struct stat{
// dev_t st_dev; /* ID of device containing file */
// ino_t st_ino; /* inode number */
// mode_t st_mode; /* 파일의 종류 및 접근권한 */
// nlink_t st_nlink; /* hardlink 된 횟수 */
// uid_t st_uid; /* 파일의 owner */
// gid_t st_gid; /* group ID of owner */
// dev_t st_rdev; /* device ID (if special file) */
// off_t st_size; /* 파일의 크기(bytes) */
// blksize_t st_blksize; /* blocksize for file system I/O */
// blkcnt_t st_blocks; /* number of 512B blocks allocated */
// time_t st_atime; /* time of last access */
// time_t st_mtime; /* time of last modification */
// time_t st_ctime; /* time of last status change */ 
// }

/* 사용자 요청(method, uri, version)을 받아 응답하는 함수 */
void doit(int fd)
{
  int is_static;                                                      // 정적파일인지 판단하는 변수
  struct stat sbuf;                                                   // 파일에 대한 정보를 갖고 있는 구조체
  
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);                                            // 빈 버퍼를 설정하고 이 버퍼와 fd 연결(connfd)
  Rio_readlineb(&rio, buf, MAXLINE);                                  // string 한 줄(Request line)을 모두 buf로 옮긴다. buf에서 메모리로 client request 읽어들이기
  printf("Request headers: \n");                                      // request header 출력
  printf("%s", buf);                                                  // 버퍼의 내용 출력
  sscanf(buf, "%s %s %s", method, uri, version);                      // buf에 있는 데이터를 method, uri, version에 담기
  // Homework 11.11
  #ifdef homework_11_11
    if (strcasecmp(method, "GET")!=0 && strcasecmp(method, "HEAD")!=0)  
    { // method가 GET이 아니거나 HEAD가 아니면 error 메세지 출력
      clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
      return;
    }
  #else
    if (strcasecmp(method, "GET")!=0)  
    { // method가 GET이 아니면 error 메세지 출력
      clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
      return;
    }
  #endif
  // read_requesthdrs(&rio);

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);      // uri를 이용해 filename과 cgiargs 찾고 정적 콘텐츠인지, 동적 콘텐츠인지 구분  
                                                      // 여기서 filename: 클라이언트가 요청한 서버의 컨텐츠 디렉토리 및 파일 이름    
  if (stat(filename, &sbuf) < 0)                      // file과 맞는 정보 조회를 못했으면(=파일정보를 구조체에 복사하지 못했으면) -1 반환-> error 메세지 출력 
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }
  /* Serve static content */
  if (is_static)                                      // 정적 콘텐츠라면 serve_static 함수 call 
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {                                                 // file이 정규파일이 아니거나 사용자 읽기가 안되면 error 메세지 출력
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size, method); // Response header에 Content-length 위해 파일의 사이즈를 같이 보낸다
  }
  /* Serve dynamic content */
  else                                                // 동적 콘텐츠라면 serve_dynamic 함수 call 
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {                                                 // file이 정규파일이 아니거나 사용자 읽기가 안되면 error 메세지 출력
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs, method);     // 인자를 같이 보낸다
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body (HTML 형식) */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}
/* read_requesthdrs: 클라이언트가 버퍼 rp에 보낸 나머지 요청 헤더들을 무시한다.(그냥 프린트)*/
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while(strcmp(buf, "\r\n"))
  { // 버퍼 rp의 마지막 끝을 만날 때까지(마지막 \r\n") 계속 출력해줘서 없앤다.
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}
/* parse_usi 함수는 uri를 받아 요청받은 파일의 이름(filename)과 요청 인자(cgiarg)를 채워준다. */
int parse_uri(char *uri, char *filename, char *cgiargs)
{ /* 정적콘텐츠는 1, 동적 콘텐츠는 0 반환 */
  char *ptr;
  #ifdef homework_11_10
  /* Static content */
    if (!strstr(uri, "cgi-bin"))        //uri에 cgi-bin이 없으면
    {
      strcpy(cgiargs, "");
      strcpy(filename, ".");
      strcat(filename, uri);
      if (uri[strlen(uri)-1] == '/')    // uri: /
        strcat(filename, "home.html");  // filename: ./home.html 
      else
      { 
        uri = uri + strlen(uri) -5;     
        if (!strcasecmp(uri, "adder"))  // uri: /adder
          strcat(filename, ".html");    // filename: adder.html
      }  
      return 1;                         
    }
    /* Dynamic content */
    else                                // uri에 cgi-bin이 있으면
    {
      ptr = index(uri, '?');            // index 함수는 uri에서 ?를 찾고, ?를 가리키는 포인터를 반환
      
      if (ptr)                          // ?가 있다면    
      {
        strcpy(cgiargs, ptr+1);         // ? 다음의 문자들을 cgiargs로 복사 (uri: /cgi-bin/new_adder?fnum=3&snum=7) ->cgiargs: fnum=3&snum=7 (uri: /cgi-bin/adder?3&7) ->cgiargs: 3&7
        *ptr = '\0';                    // ?를 NULL 문자로 바꿈
      }
      else                              // ?가 없다면
        strcpy(cgiargs, "");            // cgiargs는 없음
      strcpy(filename, ".");            
      strcat(filename, uri);            // filename: ./cgi-bin/new_adder  filename: ./cgi-bin/adder
      return 0;
    }
  #else
    if (!strstr(uri, "cgi-bin"))        // uri에 cgi-bin이 없으면
    {
      strcpy(cgiargs, "");
      strcpy(filename, ".");
      strcat(filename, uri);
      if (uri[strlen(uri)-1] == '/')    // uri: /
        strcat(filename, "home.html");  // filename: ./home.html   
      return 1;                         
    }
    /* Dynamic content */
    else                                // uri에 cgi-bin이 있으면
    {
      ptr = index(uri, '?');            // index 함수는 uri에서 ?를 찾고, ?를 가리키는 포인터를 반환
      
      if (ptr)                          // ?가 있다면    
      {
        strcpy(cgiargs, ptr+1);         // ? 다음의 문자들을 cgiargs로 복사 (uri: /cgi-bin/adder?3&7) -> cgiargs: 3&7
        *ptr = '\0';                    // ?를 NULL 문자로 바꿈
      }
      else                              // ?가 없다면
        strcpy(cgiargs, "");            // cgiargs는 없음
      strcpy(filename, ".");            
      strcat(filename, uri);            // filename: ./cgi-bin/adder  
      return 0;
    }
  #endif
}
/* serve_static은 filename을 받아서 정적 컨텐츠에 대한 요청에 대한 응답을 수행하는 함수 */
void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  /* Send response headers to client */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");                        // buf에 해당 문자열(HTTP/1.0 200 OK) 저장
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);         // buf에 Server: Tiny Web Server 저장하고 출력
  sprintf(buf, "%sConnection: close\r\n", buf);               // buf에 Conndection: close 저장하고 출력
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);    // buf에 Content-length: [filesize] 저장하고 출력
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);  // buf에 Content-type: [filetype] 저장하고 출력
  
  Rio_writen(fd, buf, strlen(buf));                           // 서버 메모리에 buf 내용 써줌 Rio_writen 없으면 밑에 printf("%s", buf) 출력 안됌
  printf("Response headers: \n");                             // 밑에 지우게 되면 클라이언트한테 보낸 내용이 서버 출력에 안뜸
  printf("%s", buf);

  if (strcasecmp(method, "GET") == 0)                         // Homework 11.11 method가 GET일 때만 body 보냄
  {/* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);
    #ifdef homework_11_9
    // Homework 11.9: 정적 컨텐츠 처리할 때 요청 파일 Mmap, Munmap 대신 malloc, free, rio_readn 사용하여 연결 식별자에게 복사
      srcp = (char *)malloc(filesize);
      Rio_readn(srcfd, srcp, filesize);
      Close(srcfd);
      Rio_writen(fd, srcp, filesize);
      free(srcp);
    #else 
      srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
      Close(srcfd);
      Rio_writen(fd, srcp, filesize);
      Munmap(srcp, filesize);
    #endif
  }
}
/* 
  serve_dynamic은 filename을 받아서 동적 컨텐츠에 대한 요청에 대한 응답을 수행하는 함수 
  1. fork()를 실행하면 부모 프로세스와 자식 프로세스가 동시에 실행된다.
  2. 만약 fork()의 반환값이 0이라면, 즉 자식 프로세스가 생성됐으면 if문을 수행한다. 
  3. fork()의 반환값이 0이 아니라면, 즉 부모 프로세스라면 if문을 건너뛰고 Wait(NULL) 함수로 간다. 이 함수는 부모 프로세스가 먼저 도달해도 자식 프로세스가 종료될 때까지 기다리는 함수이다.
  4. if문 안에서 setenv 시스템 콜을 수행해 "QUERY_STRING"의 값을 cgiargs로 바꿔준다. 우선순위가 1이므로 기존의 값과 상관없이 값이 변경된다.
  5. Dup2 함수를 실행해서 CGI 프로세스의 표준 출력을 fd(서버 연결 소켓 식별자)로 복사한다. 이제 STDOUT_FILENO의 값은 fd이다. 다시 말해, CGI 프로세스에서 표준 출력을 하면 그게 서버 연결 식별자를 거쳐 클라이언트에 출력된다.
  6. execuv 함수를 이용해 파일 이름이 filename인 파일을 실행한다.
*/
void serve_dynamic(int fd, char *filename, char * cgiargs, char *method)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));
  // fork(): 함수를 호출한 프로세스를 복사하는 기능
  // 부모 프로세스(원래 진행되던 프로세스), 자식 프로세스(복사된 프로세스)
  // Tiny는 자식 프로세스를 fork하고, CGI 프로그램을 자식에서 실행하여 동적 컨텐츠를 표준 출력으로 보냄
  if(Fork() == 0)
  { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    // method를 cgi-bin/new_adder.c에 넘겨주기 위해 환경변수 set
    setenv("REQUEST_METHOD", method, 1);
    Dup2(fd, STDOUT_FILENO);  /* Redirect stdout to client */
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  Wait(NULL);
}

/* 
 * get_filetype - Derive file type from filename
 */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))  strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))  strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))  strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))  strcpy(filetype, "image/jpeg");  
  // homework 11.7
  #ifdef homework_11_7  
  else if (strstr(filename, ".mp4"))  strcpy(filetype, "video/mp4");
  else if (strstr(filename, ".mpg"))  strcpy(filetype, "video/mpg");
  #endif
  else  strcpy(filetype, "text/plain");
}