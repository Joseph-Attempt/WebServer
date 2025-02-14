 #include <stdio.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <string.h>
 #include <netdb.h>
 #include <sys/types.h> 
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 
 #define BUFSIZE 2048
 #define FILENAMESIZE 40
 #define FILETYPESIZE 5
 #define LISTENQ 10
 #define HTTP_REQUEST_FILENAME_START_POSTION 5
 #define WORKINGDIRECTORYSIZE 1000

 
 /*
  * error - wrapper for perror
  */
 void error(char *msg) {
   perror(msg);
   exit(1);
 }
 
 int grabFileName(char http_header[BUFSIZ], char file_name[FILENAMESIZE]) {
    char *http_substr = "HTTP";
    char *http_substr_pos = strstr(http_header, http_substr);
    if (http_substr_pos == NULL) printf("HTTP Request is Malformed\n");//NOTE: PUT ERROR CODE
    strncpy(file_name, http_header + HTTP_REQUEST_FILENAME_START_POSTION, http_substr_pos - http_header - HTTP_REQUEST_FILENAME_START_POSTION - 1);
    printf("FileName: %s\n", file_name);
    return 0;
}

// NOTE: This does not account for / or /inside 
int grabFileType(char http_header[BUFSIZ], char file_type[FILETYPESIZE]) {
    char *dot_substr = ".";
    char *http_substr = "HTTP";
    char *http_substr_pos = strstr(http_header, http_substr);
    char *dot_substr_pos = strstr(http_header, dot_substr);
    if (http_substr_pos == NULL) printf("HTTP Request is Malformed\n");//NOTE: PUT ERROR CODE
    if (dot_substr_pos == NULL) printf("HTTP Request is Malformed\n");//NOTE: PUT ERROR CODE
    strncpy(file_type, http_header +(int)(dot_substr_pos - http_header) +1, (int)(http_substr_pos - dot_substr_pos) -2);
    return 0;
}

int grabConnectionStatus() {
    return 0;
}
 
 int main(int argc, char **argv) {
   int sockfd; 
   int listenfd, connfd;
   int portno; 
   int clientlen; 
   struct sockaddr_in serveraddr; 
   struct sockaddr_in clientaddr;
   struct hostent *hostp;
   char buf[BUFSIZE]; 
   char *hostaddrp;
   int optval;
   int n; 
   FILE *server_response;
   char filename[FILENAMESIZE];
   char file_type[FILETYPESIZE];
   int position;
   int file_status;
   FILE *fp; 

   if (argc != 2) {
     fprintf(stderr, "usage: %s <port>\n", argv[0]);
     exit(1);
   }
   portno = atoi(argv[1]);
 
   listenfd = socket(AF_INET, SOCK_STREAM, 0);
   if (listenfd < 0) error("ERROR opening socket\n");
 
   optval = 1;
   setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
 
   bzero((char *) &serveraddr, sizeof(serveraddr));
   serveraddr.sin_family = AF_INET;
   serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
   serveraddr.sin_port = htons((unsigned short)portno);
 
   if (bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) error("ERROR on binding\n");
   listen (listenfd, LISTENQ);


   clientlen = sizeof(clientaddr);
 
   while (1) {

    bzero(buf, BUFSIZE);
    connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen );
    printf("connection from %s, port %d\n", inet_ntop(AF_INET, &clientaddr.sin_addr, buf, sizeof(buf)), ntohs(clientaddr.sin_port) );
    n = recvfrom(connfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0) error("ERROR in recvfrom\n");
    printf("buf Below:\n%s\n", buf);
    

    grabFileName(buf, filename);

    grabFileType(buf, file_type);

    bzero(buf, BUFSIZE);
    strcpy(buf, "Goodbye Client!\n");
    n = sendto(connfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
    
    if (n < 0) error("ERROR in sendto\n");
    close(connfd);
     
     bzero(buf, BUFSIZE);
     bzero(filename, FILENAMESIZE);    
     bzero(file_type, FILETYPESIZE);    
     
 
   }
 
   return 0;
 }

 /*
 example keep alive http header

GET /index.html HTTP/1.1
Host: localhost
Connection: Keep-alive



If the requested URL is a directory (ie: GET / HTTP/1.1 or GET /inside/ HTTP/1.1), 
the web server should attempt to find a default page named index.html or index.htm in the requested directory. 

If the client sends a HTTP/1.0 request, the server must respond back with a HTTP/1.0 protocol in its reply.  Similarly for the HTTP/1.1 protocol.
 */