/*
Created by Akshay Parkala

For COEN 317 Distributed Systems

TODO:   Implement a web server which accepts HTTP GET requests and sends
        corresponding response
*/

#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#define MAXTHREADS 100

char* DOC_ROOT;

extern int errno;

void check(int no, char* msg){
  if (no < 0){
    puts(msg);
    exit(0);
  } else {
    return;
  }
}

int create_socket(int portno){
  printf("\n\n-----------------Creating Socket-------------------\n\n");
  int sockfd;
  check((sockfd = socket(AF_INET, SOCK_STREAM, 0)), "Error creating socket:");

  struct sockaddr_in serv_addr;

  uint16_t port = portno;

  while (1) {
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      port++;
    } else {
      break;
    }
  }

  check(listen(sockfd, SOMAXCONN), "Error listening on socket");
  printf("Running on port: %d\n", port);

  return sockfd;
}

int getHttpRequest(int sockfd, char *buf){
  int numbytes;

  numbytes = recv(sockfd, buf, 1023, 0);

  if (numbytes == -1) {
    if ((errno == EAGAIN) | (errno == EWOULDBLOCK)) {
      printf("\n\n!!!RECV Timeout reached on socket no: %d!!!\n\n", sockfd);
    }
    return 0;
  }
  buf[numbytes] = '\0';
  printf("numbytes: %d\n", numbytes);
  return 1;
}

int checkMethod (char *method) {
  printf("\n\n-----------------Checking Method-------------------\n\n");
  if (strcmp(method, "GET") != 0){
    return -1;
  } else {
    return 1;
  }
}

int isDirectoryExists(const char *path) {
  printf("\n\n-----------------Checking if directory exists-------------------\n\n");
    struct stat stats;

    stat(path, &stats);

    // Check for file existence
    if (S_ISDIR(stats.st_mode))
        return 1;

    return 0;
}

int checkFilename (char *filename) {
  printf("\n\n-----------------Checking requested file status-------------------\n\n");
  char filepath[1024];

  strcpy(filepath, DOC_ROOT);
  strcat(filepath, filename);

  if (isDirectoryExists(filepath) == 1) return 4;

  if (access(filepath, F_OK) == 0) {
    if (access(filepath, R_OK) == 0) {
      return 1;
    } else {
      return 3;
    }
  } else {
      return 2;
    }
}


char *read_file(FILE *fpipe) {
  printf("\n\n-----------------Reading content of requested file-------------------\n\n");
  int capacity = 10;
  char *buf = malloc(capacity);
  int index = 0;

  int c;
  while ((c = fgetc(fpipe)) != EOF) {
    assert(index < capacity);
    buf[index++] = c;

    if (index == capacity) {
      char *newbuf = malloc(capacity * 2);
      memcpy(newbuf, buf, capacity);
      free(buf);
      buf = newbuf;
      capacity *= 2;
    }
  }
  // TODO(petko): Test with feof, ferror?

  buf[index] = '\0';
  return buf;
}

unsigned char *read_image(FILE* fp){

  fseek(fp, 0L, SEEK_END);
  int size = ftell(fp);

  rewind(fp);

  unsigned char *buf = (unsigned char *)malloc(size+1);

  int c = fread(buf, 1, size, fp);
  buf[size] = '\0';
  return buf;
}

char *getFileType (char *filename) {
  char *buf = (char *)malloc(12*sizeof(char));
  printf("\n\n-----------------Getting file type-------------------\n\n");

  if (strstr(filename, "html")) {
    buf = "text/html";
  } else if (strstr(filename, "jpeg")) {
    buf = "image/jpeg";
  } else if (strstr(filename, "gif")) {
    buf = "image/gif";
  } else if (strstr(filename, "png")) {
    buf = "image/png";
  } else if (strstr(filename, "txt")) {
    buf = "text/plain";
  } else if (strstr(filename, "jpg")) {
    buf = "image/gif";
  }

  return buf;
}

char *buildHeader(int file_status, char* filename, int content_length){
  char buf[1024], length_str[100];
  char *header;

  char *filetype;
  filetype = getFileType(filename);

  sprintf(length_str, "%d", content_length);

  switch(file_status){
    case 1:
      strcpy(buf, "HTTP/1.1 200 OK\n");
      strcat(buf, "Content-Type: ");
      strcat(buf, filetype);
      strcat(buf, "\n");
      break;
    case 2:
      strcpy(buf, "HTTP/1.1 404 File Not Found\n");
      strcat(buf, "Content-Type: text/html\n");
      break;
    case 3:
      strcpy(buf, "HTTP/1.1 403 Access denied - no permissions\n");
      strcat(buf, "Content-Type: text/html\n");
      break;
    case 4:
      strcpy(buf, "HTTP/1.1 400 Bad Request - cannot request a directory\n");
      strcat(buf, "Content-Type: text/html\n");
      break;
    case 5:
      strcpy(buf, "HTTP/1.1 400 Bad Request - cannot request a directory\n");
      strcat(buf, "Content-Type: text/html\n");
      break;
    }

  strcat(buf, "Content-Length: ");
  strcat(buf, length_str);
  strcat(buf, "\n");

  strcat(buf, "Connection: close");
  strcat(buf, "\n");

  time_t now = time(&now);
  if (now == -1) {
    puts("The time() function failed");
  }
  struct tm *ptm = gmtime(&now);
  if (ptm == NULL) {
    puts("The gmtime() function failed");
  }

  char *time = asctime(ptm);

  strcat(buf, "Date: ");
  strcat(buf, time);
  strcat(buf, "\n");




  header = (char *)malloc((strlen(buf)+1)*sizeof(char));
  sprintf(header, "%s", buf);

  return header;

}

void* getContent(int file_status, char* filename){

  char *temp;
  FILE *fp;
  char fpath[1024];
  char* filetype;
  char* content;
  unsigned char* image_content;

  strcpy(fpath, DOC_ROOT);
  strcat(fpath, filename);

  filetype = getFileType(filename);

  switch(file_status){
    case 1:
      if (strstr(filetype, "image")){
        fp = fopen(fpath, "rb");
        image_content = read_image(fp);
        fclose(fp);
      } else {
        fp = fopen(fpath, "r");
        content = read_file(fp);
        fclose(fp);
      }
      break;
    case 2:
      content = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>404 Not Found</title>\n</head>\n<body>\n<h1>Not Found</h1>\n<p>The requested URL was not found on this server.</p>\n</body>\n</html>";
      break;
    case 3:
      content = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>403 Access Denied</title>\n</head>\n<body>\n<h1>Access Denied</h1>\n<p>You are not authorized to access the requested file: necessary permissions are not granted.</p>\n</body>\n</html>";
      break;
    case 4:
      content = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>400 Bad Request</title>\n</head>\n<body>\n<h1>Bad Request</h1>\n<p>A directory cannot be requested.</p>\n</body>\n</html>";
      break;
    case 5:
      content = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>400 Bad Request</title>\n</head>\n<body>\n<h1>Bad Request</h1>\n<p>This server only handles GET requests.</p>\n</body>\n</html>";
      break;
    }

    if((strstr(filetype, "image") != NULL) & (file_status == 1)){
      return (void *) image_content;
    }
    return (void *)content;
}

void sendResponse (int sockfd, int file_status, char *filename) {
  printf("\n\n-----------------Sending Response-------------------\n\n");

  unsigned char* image_content;
  char *content, *header;
  char* filetype;
  int numbytes, filesize;
  FILE *fp;
  char fpath[1024];
  strcpy(fpath, DOC_ROOT);
  strcat(fpath, filename);

  filetype = getFileType(filename);

  if ((strstr(filetype, "image") != NULL) & (file_status == 1)){
    image_content = (unsigned char*)getContent(file_status, filename);
    fp = fopen(fpath, "rb");
    fseek(fp, 0L, SEEK_END);
    filesize = ftell(fp);
    fclose(fp);
  } else {
    content = (char *)getContent(file_status, filename);
    filesize = strlen(content);
  }

  puts(content);
  header = buildHeader(file_status, filename, filesize);
  //printf("Socket: %d", sockfd);
  //puts(header);


  if((numbytes = send(sockfd, header, strlen(header), 0)) == -1) {
    printf("\n\nError while sending HTTP response header.\n");
    exit(0);
  }
  printf("Sent header");

  if((strstr(filetype, "image") != NULL) & (file_status == 1)){
    if((numbytes = send(sockfd, image_content, filesize, 0)) == -1) {
      printf("\n\nError while sending HTTP response content.\n");
      exit(0);
    }
  } else {
    if((numbytes = send(sockfd, content, strlen(content), 0)) == -1) {
      printf("\n\nError while sending HTTP response content.\n");
      exit(0);
    }
  }
  return;
}


void *connectionHandler (void *sfd) {
  int sockfd = * (int *)sfd;
  printf("\n\n-----------------Handling received connection on socket no: %d-------------------\n\n", sockfd);


  char buf[1024], *first_line, *method, filename[1024], *initial;
  int file_status;


  int status = getHttpRequest(sockfd, buf);

    char *req = strdup(buf);

    first_line = strsep(&req, "\n");

    method = strsep(&first_line, " ");

    initial = strsep(&first_line, " ");
    strcpy(filename, initial);

    if (strcmp(filename, "/") == 0) {
      strcpy(filename, "/index.html");
    }

    printf("\n HTTP method: %s", method);
    printf("\n Requested file: %s", filename);

    if (checkMethod(method) == -1){
      file_status = 5;
    } else {
      file_status = checkFilename(filename);
    }


  sendResponse(sockfd, file_status, filename);
  close(sockfd);
  pthread_exit(NULL);
  //pthread_exit(NULL);

}


int main(int argc, char* argv[]){

  //Arguments check
  if(argc != 3) {
    puts("Exactly 2 arguments, document root and port number, in that order, must be entered.");
    exit(0);
  }

  //Retrieving document root and port no
  int len = strlen(argv[1]);
  DOC_ROOT = (char*)malloc(len*sizeof(char));
  sprintf(DOC_ROOT, "%s", argv[1]);
  int portno;
  sprintf(DOC_ROOT, "%s", argv[1]);
  portno = atoi(argv[2]);

  printf("\nPort number entered: %d", portno);
  //creating a listening socket
  int sockfd = create_socket(portno);

  //initializing client address
  struct sockaddr_in client_addr;
  int cli_len = sizeof(client_addr);

  int pthread_ctr = 0;
  pthread_t **thread_pool = (pthread_t **)malloc(MAXTHREADS * sizeof(pthread_t*));


  for (pthread_ctr = 0; pthread_ctr < MAXTHREADS; pthread_ctr++) {
    int newsockfd;
    check((newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, (socklen_t *) &cli_len)), "Error accepting connection");
    printf("\nNew socket: %d\n", newsockfd);

    thread_pool[pthread_ctr] = (pthread_t *) malloc(sizeof(pthread_t));

    if (pthread_create(thread_pool[pthread_ctr], NULL, connectionHandler, &newsockfd)<0) {
      puts("Error creating thread");
      exit(0);
    };

  }

  pthread_join(*thread_pool[99], NULL);
  close(sockfd);

}


//  <link href="/media/scuedu/style-assets/stylesheets/f.css" rel="stylesheet">
//484 <a href="https://www.scu.edu/real/" class="w-100 h-auto"><img class="hp-story-img w-100 h-auto img-fluid" src="/media/institutional-pages/news-amp-events/real-data-collection.jpg" alt="Getting REAL Experience"></a>
