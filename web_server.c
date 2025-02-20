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
 #define WORKINGDIRECTORYPATHSIZE 1000
 #define HTTPVERSIONSIZE 10

 
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
    printf("file_type: %s\n", file_type);
    return 0;
}

int grabConnectionStatus() {
    return 0;
}

int grabHTTPVersion(char http_header[BUFSIZ], char http_version[HTTPVERSIONSIZE]){
    char *http_substr = "HTTP";
    char *http_substr_pos = strstr(http_header, http_substr);
    if (http_substr_pos == NULL) printf("HTTP Request is Malformed\n");//NOTE: PUT ERROR CODE
    strncpy(http_version, http_substr_pos, 8);
    printf("http_version: %s\n", http_version);
    return 0;
}

int determineResponseContentType(char file_type[FILETYPESIZE], char content_type[100]){
    if (strcmp(file_type, "html") == 0) {
        printf("Content type is html!.\n");
    } else if (strcmp(file_type, "txt") == 0) {
        printf("Content type is txt!.\n");
    } else if (strcmp(file_type, "png") == 0) {
        printf("Content type is png.\n");
    }  else if (strcmp(file_type, "gif") == 0) {
        printf("Content type is gif.\n");
    }  else if (strcmp(file_type, "jpg") == 0) {
        printf("Content type is jpg.\n");
    }  else if (strcmp(file_type, "ico") == 0) {
        printf("Content type is ico.\n");
    }  else if (strcmp(file_type, "css") == 0) {
        printf("Content type is css.\n");
    } else if (strcmp(file_type, "js") == 0) {
        printf("Content type is js.\n");
    } else {
        printf("Invalid selection.\n");
    }

    return 0;

}

int buildHTTPResponseHeader(char http_header[BUFSIZ], char http_version[HTTPVERSIONSIZE]) {
    char file_type[FILETYPESIZE]; 
    char content_type[100];
    char version[100] = "HTTP/";
    // char content_type[100] = "Content-Type: ";
    char content_length[100] = "Content-Length: ";
    
    grabFileType(http_header, file_type);



    /*
    Should look like: 
HTTP/1.1 200 OK
Content-Type: <>
Content-Length: <>
Connection: Keep-alive
\r\n\r\n
<file contents>

    */
}


int serveFile(){
/*
    char cwd[WORKINGDIRECTORYPATHSIZE];
    if (getcwd(cwd, WORKINGDIRECTORYPATHSIZE) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
        return 1;
    }
*/    
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
   char http_version[HTTPVERSIONSIZE];
   char responseType[100];
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
    grabHTTPVersion(buf, http_version);
    determineResponseContentType(file_type, responseType);

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