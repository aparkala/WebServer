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

void getHttpRequest(int sockfd, char *buf){
  int numbytes;

  check((numbytes = recv(sockfd, buf, 1023, 0)), "Error while receiving HTTP Request");

  buf[numbytes] = '\0';
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

int checkFilename (char *filename, char *DOC_ROOT) {
  printf("\n\n-----------------Checking requested file status-------------------\n\n");
  char filepath[100];

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


void writeln_to_socket(int sockfd, const char *message) {
  write(sockfd, message, strlen(message));
  write(sockfd, "\r\n", 2);
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

char* getFileType (char *filename) {
  printf("\n\n-----------------Getting file type-------------------\n\n");
  char* filetype = (char*)malloc(15*sizeof(char));

  if (strstr(filename, "html")) {
    filetype = "text/html";
  } else if (strstr(filename, "jpeg")) {
    filetype = "image/jpeg";
  } else if (strstr(filename, "gif")) {
    filetype = "image/gif";
  } else if (strstr(filename, "png")) {
    filetype = "image/png";
  } else if (strstr(filename, "txt")) {
    filetype = "text/plain";
  } else if (strstr(filename, "jpg")) {
    filetype = "image/gif";
  }

  return filetype;
}

void sendResponse (int sockfd, int file_status, char *filename, char* DOC_ROOT) {
  printf("\n\n-----------------Sending Response-------------------\n\n");
  char header[1024], length_str[100], fpath[100];
  char *content;
  FILE *fp;

  strcpy(fpath, DOC_ROOT);
  strcat(fpath, filename);

  if (file_status == 1) { // Send 200 OK Response
    strcpy(header, "HTTP/1.1 200 OK\n");

    char *filetype = getFileType(filename);
    strcat(header, "Content-Type: ");
    strcat(header, filetype);
    strcat(header, "\n");

    if (strstr(filetype, "image")){
      fp = fopen(fpath, "rb");
      content = read_file(fp);
      fclose(fp);
    } else {
      fp = fopen(fpath, "r");
      content = read_file(fp);
      fclose(fp);
    }


    sprintf(length_str, "%d", (int)strlen(content));
    strcat(header, "Content-Length: ");
    strcat(header, length_str);
    strcat(header, "\n");

  } else if (file_status == 2) { // Send 404 BAD response
    strcpy(header, "HTTP/1.1 404 File Not Found\n");
    content = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>404 Not Found</title>\n</head>\n<body>\n<h1>Not Found</h1>\n<p>The requested URL was not found on this server.</p>\n</body>\n</html>";
  } else if (file_status == 3) { // Send 403 BAD response
    strcpy(header, "HTTP/1.1 403 Access denied - no permissions\n");
    content = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>403 Access Denied</title>\n</head>\n<body>\n<h1>Access Denied</h1>\n<p>You are not authorized to access the requested file: necessary permissions are not granted.</p>\n</body>\n</html>";
  } else if (file_status == 4) { // Send 400 BAD response
    strcpy(header, "HTTP/1.1 400 Bad Request - cannot request a directory\n");
    content = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>400 Bad Request</title>\n</head>\n<body>\n<h1>Bad Request</h1>\n<p>A directory cannot be requested.</p>\n</body>\n</html>";
  } else if (file_status == 5) { // Send 400 BAD response
    strcpy(header, "HTTP/1.1 400 Bad Request - cannot request a directory\n");
    content = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>400 Bad Request</title>\n</head>\n<body>\n<h1>Bad Request</h1>\n<p>This server only handles GET requests.</p>\n</body>\n</html>";
  }

  time_t now = time(&now);
  if (now == -1) {
    puts("The time() function failed");
  }
  struct tm *ptm = gmtime(&now);
  if (ptm == NULL) {
    puts("The gmtime() function failed");
  }

  char *time = asctime(ptm);

  strcat(header, "Date: ");
  strcat(header, time);
  strcat(header, "\n");

  int numbytes;

  if((numbytes = send(sockfd, header, strlen(header), 0)) == -1) {
    printf("\n\nError while sending HTTP response header.\n");
    exit(0);
  }

  if((numbytes = send(sockfd, content, strlen(content), 0)) == -1) {
    printf("\n\nError while sending HTTP response content.\n");
    exit(0);
  }

  return;


}


void connectionHandler (int sockfd, char* DOC_ROOT) {
  printf("\n\n-----------------Handling received connection-------------------\n\n");
  char buf[1024], *first_line, *method, filename[1024], *initial;
  int file_status;

  getHttpRequest(sockfd, buf);

  char *req = strdup(buf);

  first_line = strsep(&req, "\n");

  method = strsep(&first_line, " ");

  initial = strsep(&first_line, " ");
  strcpy(filename, initial);

  if (strcmp(filename, "/") == 0) {
    strcpy(filename, "/index.html");
  }

  printf("\n Request Method: %s", method);
  printf("\n Requested File: %s", filename);

  if (checkMethod(method) == -1){
    file_status = 5;
  } else {
    file_status = checkFilename(filename, DOC_ROOT);
  }

  sendResponse(sockfd, file_status, filename, DOC_ROOT);
  return;

}


int main(int argc, char* argv[]){

  //Arguments check
  if(argc != 3) {
    puts("Exactly 2 arguments, document root and port number, in that order, must be entered.");
    exit(0);
  }

  //Retrieving document root and port no
  char* DOC_ROOT = (char*)malloc(100*sizeof(char));
  int portno;
  sprintf(DOC_ROOT, "%s", argv[1]);
  portno = atoi(argv[2]);

  printf("\nPort number entered: %d", portno);
  //creating a listening socket
  int sockfd = create_socket(portno);

  //initializing client address
  struct sockaddr_in client_addr;
  int cli_len = sizeof(client_addr);


  while (1) {
    int newsockfd;
    check((newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, (socklen_t *) &cli_len)), "Error accepting connection");
    printf("\nNew socket: %d\n", newsockfd);

    connectionHandler(newsockfd, DOC_ROOT);

    close(newsockfd);

  }
  close(sockfd);

}


//  <link href="/media/scuedu/style-assets/stylesheets/f.css" rel="stylesheet">
//484 <a href="https://www.scu.edu/real/" class="w-100 h-auto"><img class="hp-story-img w-100 h-auto img-fluid" src="/media/institutional-pages/news-amp-events/real-data-collection.jpg" alt="Getting REAL Experience"></a>
