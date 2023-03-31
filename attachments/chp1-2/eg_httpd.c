// compiled with: gcc -static -z execstack -fno-stack-protector -no-pie
// ./eg_httpd.c -o eg_httpd
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: absolutely_safe_httpd/0.1.0\r\n"

void accept_request(int);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
int startup(u_short *);
void unimplemented(int);
void execute_para(int, char *);

/* [START] Basic startups -- */
int startup(u_short *port) {
  int httpd = 0;
  struct sockaddr_in name;
  httpd = socket(PF_INET, SOCK_STREAM, 0);
  if (httpd == -1)
    error_die("socket");
  memset(&name, 0, sizeof(name));
  name.sin_family = AF_INET;
  name.sin_port = htons(*port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
    error_die("bind");
  if (*port == 0) {
    int namelen = sizeof(name);
    if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
      error_die("getsockname");
    *port = ntohs(name.sin_port);
  }
  if (listen(httpd, 5) < 0)
    error_die("listen");
  return (httpd);
}
int main(void) {
  int server_sock = -1;
  u_short port = 0;
  int client_sock = -1;
  struct sockaddr_in client_name;
  int client_name_len = sizeof(client_name);
  pthread_t newthread;
  pid_t fk;
  server_sock = startup(&port);
  printf("httpd running on port %d\n", port);
  while (1) {
    client_sock =
        accept(server_sock, (struct sockaddr *)&client_name, &client_name_len);
    if (client_sock == -1)
      error_die("accept");
    fk = fork();
    if (fk >= 0) {
      if (!fk) {
        close(server_sock);
        accept_request(client_sock);
        exit(-1);
      }
      close(client_sock);
    }
  }
  close(server_sock);
  return 0;
}
void error_die(const char *sc) {

  perror(sc);
  exit(1);
}
/* [END] Basic startups -- */

/* [START] Basic reply -- */
void headers(int client, const char *filename) {
  char buf[1024]; // buf
  (void)filename;

  strcpy(buf, "HTTP/1.0 200 OK\r\n");
  send(client, buf, strlen(buf), 0); // str fmt?

  strcpy(buf, SERVER_STRING); // buffer overflow?
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "Content-Type: text/html\r\n");
  send(client, buf, strlen(buf), 0);
  strcpy(buf, "\r\n");
  send(client, buf, strlen(buf), 0);
}
void bad_request(int client) {
  char buf[1024];

  sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "Content-type: text/html\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "<P>Your browser sent a bad request, ");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "such as a POST without a Content-Length.\r\n");
  send(client, buf, strlen(buf), 0);
}
void not_found(int client) {
  char buf[1024];

  sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
  send(client, buf, strlen(buf), 0);

  sprintf(buf, SERVER_STRING);
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "Content-Type: text/html\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "your request because the resource specified\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "is unavailable or nonexistent.\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "</BODY></HTML>\r\n");
  send(client, buf, strlen(buf), 0);
}
void unimplemented(int client) {
  char buf[1024];

  sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
  send(client, buf, strlen(buf), 0);

  sprintf(buf, SERVER_STRING);
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "Content-Type: text/html\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "</TITLE></HEAD>\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "</BODY></HTML>\r\n");
  send(client, buf, strlen(buf), 0);
}
/* [END] Basic reply -- */

void accept_request(int client) {
  char buf[1024];
  int numchars, para_count = 0;
  char method[255];
  char url[255];
  size_t i, j;
  struct stat st;
  char *query_string = NULL;
  char path[512];

  numchars = get_line(client, buf, sizeof(buf));
  i = 0;
  j = 0;

  while (!ISspace(buf[j]) && (i < sizeof(method) - 1)) {
    method[i] = buf[j];
    i++;
    j++;
  }
  method[i] = '\0';

  if (strcasecmp(method, "GET")) {
    unimplemented(client);
    return;
  }

  i = 0;
  while (ISspace(buf[j]) && (j < sizeof(buf)))
    j++;
  while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {

    url[i] = buf[j];
    i++;
    j++;
  }
  url[i] = '\0';

  if (!strcasecmp(method, "GET")) {

    query_string = url;
    while ((*query_string != '?') && (*query_string != '\0'))
      query_string++;

    if (*query_string == '?') {
      *query_string = '\0';
      query_string++;
      para_count++;
    }
  }

  sprintf(path, "resources%s", url);
  if (path[strlen(path) - 1] == '/')
    strcat(path, "index.html");
  if (stat(path, &st) == -1) {
    while ((numchars > 0) && strcmp("\n", buf))
      numchars = get_line(client, buf, sizeof(buf));
    not_found(client);
  } else {
    if ((st.st_mode & S_IFMT) == S_IFDIR)
      strcat(path, "/index.html");
    serve_file(client, path);
  }

  if (para_count)
    execute_para(client, query_string);
  close(client);
}
void cat(int client, FILE *resource) {
  char buf[1024];
  fgets(buf, sizeof(buf), resource);
  while (!feof(resource)) {
    send(client, buf, strlen(buf), 0);
    fgets(buf, sizeof(buf), resource);
  }
}
void flag1(int client) {
    int fd1 = open("./resources/flag1", 0);
    char buf[0xd0];
    strcpy(buf, "Good job! Here's your flag: ");
    send(client, buf, strlen(buf), 0);
    sendfile(client, fd1, 0, 64);
    strcpy(buf, "Let me give you another gift: ");
    send(client, buf, strlen(buf), 0);
    asm("pop %rax");
    sprintf(buf, "%p", &buf);
    send(client, buf, strlen(buf), 0);
}
void execute_para(int client, char *paras) {
  char buf[0xd0];
  sprintf(buf, "Hello, ");
  send(client, buf, strlen(buf), 0);
  strcpy(buf, &paras[5]);
  send(client, buf, strlen(buf), 0);
}
int get_line(int sock, char *buf, int size) {
  int i = 0;
  char c = '\0';
  int n;

  while ((i < size - 1) && (c != '\n')) {
    n = recv(sock, &c, 1, 0);
    if (n > 0) {
      if (c == '\r') {
        n = recv(sock, &c, 1, MSG_PEEK);
        if ((n > 0) && (c == '\n'))
          recv(sock, &c, 1, 0);
        else
          c = '\n';
      }
      buf[i] = c;
      i++;
    } else
      c = '\n';
  }
  buf[i] = '\0';
  return (i);
}

void serve_file(int client, const char *filename) {
  FILE *resource = NULL;
  int numchars = 1;
  char buf[1024];

  buf[0] = 'A';
  buf[1] = '\0';
  while ((numchars > 0) && strcmp("\n", buf))
    numchars = get_line(client, buf, sizeof(buf));

  resource = fopen(filename, "r");
  if (resource == NULL)
    not_found(client);
  else {
    headers(client, filename);
    cat(client, resource);
  }
  fclose(resource);
}
