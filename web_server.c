 #include <stdio.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <string.h>
 #include <netdb.h>
 #include <sys/types.h> 
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <sys/stat.h>
 
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
    printf("In grabFileName FileName: %s\n", file_name);
    return 0;
}

// NOTE: This does not account for / or /inside 
//NOTE: THIS DOES NOT ACCOUNT FOR FILES WITH MULTIPLE DOTS, such as the jquery file in wwww
int grabFileType(char http_header[BUFSIZ], char file_type[FILETYPESIZE]) {
    char *dot_substr = ".";
    char *http_substr = "HTTP";
    char *http_substr_pos = strstr(http_header, http_substr);
    char *dot_substr_pos = strstr(http_header, dot_substr);
    if (http_substr_pos == NULL) printf("HTTP Request is Malformed\n");//NOTE: PUT ERROR CODE
    if (dot_substr_pos == NULL) printf("HTTP Request is Malformed\n");//NOTE: PUT ERROR CODE
    strncpy(file_type, http_header +(int)(dot_substr_pos - http_header) +1, (int)(http_substr_pos - dot_substr_pos) -2);
    // printf("file_type: %s\n", file_type);
    return 0;
}

int determineConnectionStatus(char http_header[BUFSIZ], char http_connection_status[40]) {
// NOTE: Need to plan for the a in Keep-Alive being capitalized
    char *connection_str = "Keep-alive:";
    char *connection_str_pos = strstr(http_header, connection_str);
    if (connection_str_pos == NULL){
        strncpy(http_connection_status, "Connection: Close", 17);  
    }else {
        strncpy(http_connection_status, "Connection: Keep-Alive", 23);  
    }

    // printf("Connection: [Status]: %s\n", http_connection_status);

    /*
GET /index.html HTTP/1.1
Host: localhost
Connection: Keep-alive
    */
    return 0;
}

int grabHTTPVersion(char http_header[BUFSIZ], char http_version[HTTPVERSIONSIZE]){
    char *http_substr = "HTTP";
    char *http_substr_pos = strstr(http_header, http_substr);
    if (http_substr_pos == NULL) printf("HTTP Request is Malformed\n");//NOTE: PUT ERROR CODE
    strncpy(http_version, http_substr_pos, 8);
    sprintf(http_version, "%s 200 OK",http_version);
    // HTTP/1.1 is the result of the above
    printf("http_version: %s\n", http_version);
    return 0;
}

int determineResponseContentType(char file_type[FILETYPESIZE], char content_type[100]){
    if (strcmp(file_type, "html") == 0) {
        strncpy(content_type, "Content-Type: text/htlm", sizeof("Content-Type: text/htlm"));
    } else if (strcmp(file_type, "txt") == 0) {
        strncpy(content_type, "Content-Type: text/plain", sizeof("Content-Type: text/plain"));
    } else if (strcmp(file_type, "png") == 0) {
        strncpy(content_type, "Content-Type: image/png", sizeof("Content-Type: image/png"));
    }  else if (strcmp(file_type, "gif") == 0) {
        strncpy(content_type, "Content-Type: image/gif", sizeof("Content-Type: image/gif"));
    }  else if (strcmp(file_type, "jpg") == 0) {
        strncpy(content_type, "Content-Type: image/jpg", sizeof("Content-Type: image/jpg"));
    }  else if (strcmp(file_type, "ico") == 0) {
        strncpy(content_type, "Content-Type: image/x-icon", sizeof("Content-Type: image/x-icon"));
    }  else if (strcmp(file_type, "css") == 0) {
        strncpy(content_type, "Content-Type: text/css", sizeof("Content-Type: text/css"));
    } else if (strcmp(file_type, "js") == 0) {
        strncpy(content_type, "Content-Type: application/javascript", sizeof("Content-Type: application/javascript"));
    } else {
        printf("WRONG selection.\n");//NOTE: Link this to error header response
        return -1;
    }

    // printf("This is the content-type going in the response header: %s\n", content_type);

    return 0;

}

// CITATION: https://dev.to/namantam1/ways-to-get-the-file-size-in-c-2mag
int determineContentLength(char content_length[100], char filename[FILENAMESIZE]) {
    struct stat file_status;
    printf("IN DETERMINE CONTENT LENGHT filename: %s\n", filename);
    if (stat(filename, &file_status) < 0) {
        printf("IN THE -1 of determineContentLenght\n");
        return -1;
    }
    sprintf(content_length, "Content-Length: %ld", file_status.st_size);
    // printf("\nThis is content_length: %s\n", content_length);
    // return file_status.st_size;
    return 0;
}

int buildHTTPResponseHeader(char response_http_header[200], char http_header[BUFSIZ], char http_version[HTTPVERSIONSIZE], char filename[FILENAMESIZE], char file_type[FILETYPESIZE], char responseType[100], char http_connection_status[40], char content_length[100]) {

    grabFileName(http_header, filename);
    printf("IN buildHTTPRESPONSEHEDER filename: %s\n", filename);
    determineContentLength(content_length, filename);


    grabFileType(http_header, file_type);
    printf("IN buildHTTPRESPONSEHEDER filename: %s\n", filename);

    grabHTTPVersion(http_header, http_version);
    printf("IN buildHTTPRESPONSEHEDER filename: %s\n", filename);

    determineResponseContentType(file_type, responseType);
    printf("IN buildHTTPRESPONSEHEDER filename: %s\n", filename);

    determineConnectionStatus(http_header, http_connection_status);
    printf("IN buildHTTPRESPONSEHEDER filename: %s\n", filename);
    // determineContentLength(content_length, filename);
    printf("\n");

    sprintf(response_http_header, "%s\r\n%s\r\n%s\r\n%s\r\n\r\nHello World!", http_version, responseType, content_length, http_connection_status);
    printf("\nTHIS IS MY RESPONSE HEADER: \n%s\n", response_http_header);

    bzero(http_header, BUFSIZE);
    bzero(http_version, HTTPVERSIONSIZE);
    bzero(filename, FILENAMESIZE);    
    bzero(file_type, FILETYPESIZE);    
    bzero(responseType,100);    
    bzero(http_connection_status, 40);
    bzero(content_length, 100);


    /*
    Should look like: 
    HTTP/1.1 200 OK
    Content-Type: text/html X
    Content-Length: 44000
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
   char http_connection_status[40];
   char content_length[100];
   char response_http_header[200];
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
   bzero(http_connection_status, 40);

   while (1) {

    bzero(buf, BUFSIZE);
    connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen );
    printf("connection from %s, port %d\n", inet_ntop(AF_INET, &clientaddr.sin_addr, buf, sizeof(buf)), ntohs(clientaddr.sin_port) );
    // n = recvfrom(connfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
    n = recv(connfd, buf, BUFSIZE, 0);
    if (n < 0) error("ERROR in recvfrom\n");
    printf("HTTP REQUEST HEADER Below:\n%s\n", buf);
    



    buildHTTPResponseHeader(response_http_header, buf, http_version, filename,  file_type, responseType,  http_connection_status, content_length);

    // n = send(connfd, buf, BUFSIZE, 0);
    n = send(connfd, response_http_header,200, 0);
    // n = sendto(connfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) error("ERROR in sendto\n");
    bzero(buf, BUFSIZE);
    // strcpy(buf, "Goodbye Client!\n");

    close(connfd);
     
    //  bzero(buf, BUFSIZE);
    //  bzero(filename, FILENAMESIZE);    
    //  bzero(file_type, FILETYPESIZE);    
    //  bzero(http_connection_status, 40);

     
 
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