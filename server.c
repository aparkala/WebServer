/*
Based on the requirements of this OS class project:
http://pages.cs.wisc.edu/~dusseau/Classes/CS537-F07/Projects/P2/p2.html
TODO: Clean up debug output.
*/

#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define CAPACITY 100
const char* GET = "GET";

struct arg {
  int sockfd;
};

void check(int no, char* msg){
  if (no < 0){
    puts(msg);
    exit(0);
  } else {
    return;
  }
}

int create_socket(int portno){
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

/*
void *connection_handler(void *args){


}
*/

void getHttpRequest(int sockfd, char *buf){
  int numbytes;
  
  check((numbytes = recv(sockfd, buf, 1023, 0)), "Error while receiving HTTP Request");

  buf[numbytes] = '\0';
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
   printf("New socket: %d\n", newsockfd);

   char buf[1024];
   getHttpRequest(newsockfd, buf);

   puts(buf);
   close(newsockfd);

 }
 close(sockfd);




}
