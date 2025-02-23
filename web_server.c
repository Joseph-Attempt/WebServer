 #include <stdio.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <string.h>
 #include <netdb.h>
 #include <sys/types.h> 
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <netinet/tcp.h>
 #include <arpa/inet.h>
 #include <sys/stat.h>
 #include <errno.h>
 #include <pthread.h>


 #define BUFSIZE 2048
 #define FILENAMESIZE 100
 #define FILETYPESIZE 100
 #define LISTENQ 24
 #define HTTP_REQUEST_FILENAME_START_POSTION 5
 #define WORKINGDIRECTORYPATHSIZE 1000
 #define HTTPVERSIONSIZE 100

 
 /*
  * error - wrapper for perror
  */
 void error(char *msg) {
   perror(msg);
   exit(1);
 }

 
 int verifyHTTPHeader(char http_header[BUFSIZE]) {
    char http_header_first_line[200];
    char *header_first_line_end_pos = strstr(http_header, "\r\n");
    //Checking that \r\n does exist. 
    //Potential errors, the string might be a http request with no \r\n at the end of the first line. \r\n might be at the end of second, third, etc. line 
    if (header_first_line_end_pos == NULL) return -1;
    strncpy(http_header_first_line, http_header, header_first_line_end_pos - http_header);
    char *http_header_first_line_verb = strtok(http_header_first_line, " ");
    char *http_header_first_line_path = strtok(NULL, " ");
    char *http_header_first_line_version = strtok(NULL, " ");
    char *should_be_null = strtok(NULL, " ");
    //Checking for only three elements in the first line of the http request header, when separated by a space
    //Potential Errors, this does not actually check if the first element is a HTTP verb, 2nd element Path, 3rd Element Version.
    if (should_be_null != NULL | http_header_first_line_verb == NULL || http_header_first_line_path == NULL || http_header_first_line_version == NULL) return -1;
    return 0;
 }


 int grabFileName(char http_header[BUFSIZE], char file_name[FILENAMESIZE], char response_http_header[200], char http_version[HTTPVERSIONSIZE]) {
    char *http_substr = "HTTP";
    char *http_substr_pos = strstr(http_header, http_substr);
    if (http_substr_pos == NULL) printf("HTTP Request is Malformed\n");//NOTE: PUT ERROR CODE
    strncpy(file_name, http_header + HTTP_REQUEST_FILENAME_START_POSTION, http_substr_pos - http_header - HTTP_REQUEST_FILENAME_START_POSTION - 1);
    if (access(file_name, R_OK) != 0) {
        if (errno == ENOENT) {
            sprintf(response_http_header, "%s 404 File Not Found\r\n\r\n404 File Not Found", http_version);
        }else if (errno == EACCES) {
            sprintf(response_http_header, "%s 403 Forbidden\r\n\r\n403 Forbidden", http_version);
        }
        return -1;
    }

    return 0;
}

// NOTE: This does not account for / or /inside 
int grabFileType(char http_header[BUFSIZE], char file_type[FILETYPESIZE]) {
    char *dot_substr = ".";
    char *http_substr = "HTTP";
    char *http_substr_pos = strstr(http_header, http_substr);
    char *dot_substr_pos = strstr(http_header, dot_substr);

    if (http_substr_pos == NULL) printf("HTTP Request is Malformed\n");//NOTE: PUT ERROR CODE for no HTTP (MALFORMED)
    if (dot_substr_pos == NULL) printf("HTTP Request is Malformed\n");//NOTE: PUT ERROR CODE for no extension (MALFORMED OR CAN't FIND FILE)
    strncpy(file_type, http_header +(int)(dot_substr_pos - http_header) +1, (int)(http_substr_pos - dot_substr_pos) -2);

   //Citation: https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
   //Below is to deal with multiple dot files, such as the jquery
    char *ext = strrchr(file_type, '.');
    if (ext) {
        strcpy(file_type, ext +1);       
    }
    return 0;
}

// NOTE: NEED TO BUILD FAILSAFE FOR VERSIONS BESIDES 1.0 / 1.1 b/c 2.0 doesn't show up in the same way on the request header
int determineConnectionStatus(char http_header[BUFSIZE], char http_connection_status[40], char http_version[HTTPVERSIONSIZE], int client_socket) {
// NOTE: Need to plan for the a in Keep-Alive and close being agnostic to capitalization
    char *keep_alive_str = ": Keep-alive";
    char *keep_alive_str_pos = strstr(http_header, keep_alive_str);
    char *close_str = ": Close";
    char *close_str_pos = strstr(http_header, close_str);

    //citation for keep-alive logic: https://stackoverflow.com/questions/17740492/how-i-will-use-setsockopt-and-getsockopt-with-keep-alive-in-linux-c-programming
    //https://linux.die.net/man/7/tcp
    int keepcnt = 5;
    int keepidle = 30;
    int keepintvl = 120;
    int optval = 1;

    if (keep_alive_str_pos != NULL)  {
        strncpy(http_connection_status, "Connection: Keep-Alive", 23);
        // setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

        // if (setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) == -1) printf("IT DID NOT WORK\n");
        // printf("WTH\n");
        setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));
        setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
        setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int));    
    } else if (close_str_pos != NULL) {
        strncpy(http_connection_status, "Connection: Close", 17);
    } else if (strncmp(http_version, "HTTP/1.0", 8) == 0) {
        strncpy(http_connection_status, "Connection: Close", 17);
    } else if (strncmp(http_version, "HTTP/1.1", 8) == 0) {
        strncpy(http_connection_status, "Connection: Keep-Alive", 23);
        // if (setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) == -1) printf("IT DID NOT WORK\n");
        // printf("WTH\n");

        setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));
        setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
        setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int));
      
    } else {
        strncpy(http_connection_status, "Connection: Unkown", 18);  
    }


    /*
GET /index.html HTTP/1.1
Host: localhost
Connection: Keep-alive
    */
    return 0;
}

int checkHTTPMethodIsGet(char http_header[BUFSIZE]) {
    char *http_get_substr = "GET ";
    char *http_get_substr_pos = strstr(http_header, http_get_substr);
    if (http_get_substr_pos == NULL) return -1;
    return 0;
}

int grabHTTPVersion(char http_header[BUFSIZE], char http_version[HTTPVERSIONSIZE]){
    char *http_substr = "HTTP";
    char *http_substr_pos = strstr(http_header, http_substr);
    if (http_substr_pos == NULL) printf("HTTP Request is Malformed\n");//NOTE: PUT ERROR CODE //THINK ABOUT CHECKING THIS in verifyinghttp method
    strncpy(http_version, http_substr_pos, 8);
    if ( (strncmp(http_version, "HTTP/1.1", 8) != 0) && (strncmp(http_version, "HTTP/1.0", 8) != 0) ) return -1;
    return 0;
}

int setStatusCode(char http_version[HTTPVERSIONSIZE], int status) {
    if (status == 0) {
        sprintf(http_version, "%s 200 OK",http_version); 
    }else if (status == 1 ){
        sprintf(http_version, "%s 400 Bad Request",http_version); 
    }else if (status == 2 ){
        sprintf(http_version, "%s 403 Forbidden",http_version); 
    }else if (status == 3 ){
        sprintf(http_version, "%s 404 Not Found",http_version);
    }else if (status == 4 ){
        sprintf(http_version, "%s 405 Method Not Allowed",http_version);
    }else if (status == 5 ){
        sprintf(http_version, "%s 505 HTTP Version Not Supported",http_version);
    }
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
        strncpy(content_type, "Content-Type: unknown", sizeof("Content-Type: unknown"));
        printf("WRONG selection.\n");//NOTE: Link this to error header response
        return -1;
    }

    // printf("This is the content-type going in the response header: %s\n", content_type);

    return 0;

}

// CITATION: https://dev.to/namantam1/ways-to-get-the-file-size-in-c-2mag
int determineContentLength(char content_length[100], char filename[FILENAMESIZE]) {
    struct stat file_status;
    if (stat(filename, &file_status) < 0) return -1;
    sprintf(content_length, "Content-Length: %ld", file_status.st_size);
    return 0;
}

int buildHTTPResponseHeader(char response_http_header[200], char http_header[BUFSIZE], char http_version[HTTPVERSIONSIZE], char filename[FILENAMESIZE], char file_type[FILETYPESIZE], char responseType[100], char http_connection_status[40], char content_length[100], int client_socket) {
    int status = 0;
    if (verifyHTTPHeader(http_header) == -1) {
        sprintf(response_http_header, "%s 400 Bad Request\r\n\r\n400 Bad Request", http_version);
        return -1;
    }else if (grabHTTPVersion(http_header, http_version) == -1){
        sprintf(response_http_header, "%s 505 HTTP Version Not Supported\r\n\r\n505 HTTP Version Not Supported", http_version);
        return -1;
    } else if (grabFileName(http_header, filename, response_http_header, http_version)) {
        return -1;
    }
    else if (checkHTTPMethodIsGet(http_header) == -1){
        //CITATION: https://medium.com/@DanSodkiewicz/simple-example-of-a-program-checking-file-access-permissions-in-c-1015e0715499
        sprintf(response_http_header, "%s 405 Method Not Allowed\r\n\r\n 405Method Not Allowed", http_version);
        return -1;
    } else {
        printf("IN ELSE \n");
        determineContentLength(content_length, filename);
        grabFileType(http_header, file_type);
        determineResponseContentType(file_type, responseType);
        setStatusCode(http_version, status);
        determineConnectionStatus(http_header, http_connection_status, http_version, client_socket);
    
        sprintf(response_http_header, "%s\r\n%s\r\n%s\r\n%s\r\n\r\n", http_version, responseType, content_length, http_connection_status);
    
    }

   return 0;
}



//CITITATION: BEEJ GUIDE 7.4 sendall function
int sendall(int s, char *buf, int len){
    int total = 0;        
    int bytesleft = len; 
    int n;
    while(total < len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    if (n == -1) return -1;
    return total;
} 

//CITATION: https://www.youtube.com/watch?v=Pg_4Jz8ZIH4
void *handle_connection(void *p_client_socket) {
    int client_socket = * ((int*)p_client_socket);
    free(p_client_socket);
    char buf[BUFSIZE]; 
    char filename[FILENAMESIZE];
    char file_type[FILETYPESIZE];
    char http_version[HTTPVERSIONSIZE];
    char responseType[100];
    char http_connection_status[40];
    char content_length[100];
    char response_http_header[200];
    FILE *fp; 
    int n; 
    int bytes_read;

    bzero(buf, BUFSIZE);
    bzero(filename, FILENAMESIZE);
    bzero(file_type, FILETYPESIZE);
    bzero(http_version, HTTPVERSIONSIZE);
    bzero(responseType, 100);
    bzero(http_connection_status, 40);
    bzero(content_length, 100);
    bzero(response_http_header, 200);


    // pthread_t thread_id = pthread_self();
    // printf("THREAD ID------------: \n%lu\n", thread_id);

    // int keepcnt = 4;
    // int keepidle = 3;
    // int keepintvl = 3;
    // setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));
    // setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
    // setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int));
    
    n = recv(client_socket, buf, BUFSIZE, 0);
    if (n < 0) error("ERROR in recvfrom\n");
    
    if(buildHTTPResponseHeader(response_http_header, buf, http_version, filename,  file_type, responseType,  http_connection_status, content_length, client_socket) == -1) {
        n = send(client_socket, response_http_header, strlen(response_http_header), 0);
        return NULL;
    }
    
    fp = fopen(filename, "r"); //Checking for file existense and readability happens in buildHTTPResponseHeader
    n = send(client_socket, response_http_header, strlen(response_http_header), 0);
    printf("HTTP REQUEST HEADER Below:\n%s\n", buf);
    printf("This is the RESPONSE header: \n%s\n", response_http_header);    
    bzero(buf, BUFSIZE);
    
    while (1){
      bytes_read = fread(buf, 1, BUFSIZE, fp);
      if (bytes_read < 1) {
        break;
      }
      if (sendall(client_socket, buf, BUFSIZE) == -1) {
        error("Error in sending file data");
      }
      bzero(buf, BUFSIZE);
    } 


    close(client_socket);
    fclose(fp);

    bzero(buf, BUFSIZE);
    bzero(http_version, HTTPVERSIONSIZE);
    bzero(filename, FILENAMESIZE);    
    bzero(file_type, FILETYPESIZE);    
    bzero(responseType,100);    
    bzero(http_connection_status, 40);
    bzero(content_length, 100);


    return NULL;
}
 

 int main(int argc, char **argv) {
   int listenfd, connfd;
   int portno; 
   int clientlen;
   struct sockaddr_in serveraddr; 
   struct sockaddr_in clientaddr;
   struct hostent *hostp;
   char *hostaddrp;
   int optval;
   int n; 
   char buf[BUFSIZE]; 

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
    connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen );
    printf("\nconnection from %s, port %d\n", inet_ntop(AF_INET, &clientaddr.sin_addr, buf, sizeof(buf)), ntohs(clientaddr.sin_port) );
    //CITATION: https://www.youtube.com/watch?v=Pg_4Jz8ZIH4
    pthread_t t;
    int *pclient = malloc(sizeof(int));
    *pclient = connfd;
    pthread_create(&t, NULL, handle_connection, pclient);
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

 */