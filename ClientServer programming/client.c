#define _POSIX_C_SOURCE 201712L
#define MAXLINE 256
#define MAXARGS 256
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


int open_clientfd(char *hostname, char *port) {
    int clientfd;
    struct addrinfo hints, *listp, *p;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV; /* …using numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
    getaddrinfo(hostname, port, &hints, &listp);


    /* Walk the list for one that we can successfully connect to */
    for(p = listp; p; p = p->ai_next) {
        // reminder to self: p is a pointer

        /* Create a socket descriptor*/
        if((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue; /* Socket failed, try the next*/ 
        }

        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) {
            break; /* Success */
        }
            
        close(clientfd); /* Connect failed, try another */
    }

    /* Clean up */
    freeaddrinfo(listp);
    if (!p){
         /* All connects failed */
         return-1;
    } else {
        /* The last connect succeeded */
        return clientfd;
    }
}
void clearArgv(char *argv[MAXARGS])
{
    for (int i = 0; i < MAXARGS; i++)
    {
        argv[i] = NULL;
    }
}
int parseline(char inputCopy[MAXLINE], char *argv[MAXARGS])
{
    clearArgv(argv);
    char *token = strtok(inputCopy, " ,\n");
    int i = 0;
    while (token != NULL)
    {
        argv[i] = token;
        i++;
        token = strtok(NULL, " ,\n");
    }
    return i; // size of argv
}
int checkDateFormat(char* date){
    int year, month, day;

    if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3) return 1; //1 is false
    if (month < 1 || month > 12 || day < 1 || day > 31 || year < 1000) return 1;
    return 0; 
}
void client_echo(int clientfd) {
    size_t n;
    char buf[MAXLINE];

    while (1) {
        printf("> ");  // Display prompt
        fflush(stdout);
        if (fgets(buf, MAXLINE, stdin) == NULL) {
            break;  // Exit the loop if there's an error or EOF
        }
        char *getargv[MAXARGS];
        char copybuf[MAXLINE];
        strcpy(copybuf, buf);
        int size_of_argv = parseline(copybuf, getargv);
        int isValid=0;
        //check whether the command is vaild
        if((strncmp(buf, "Prices", 6) == 0 && size_of_argv!=3)||
        (strncmp(buf, "MaxProfit", 9) == 0 && size_of_argv!= 4)||
        (
        strncmp(buf, "List", 4) != 0 &&
        strncmp(buf, "Prices", 6) != 0&&
        strncmp(buf, "MaxProfit", 9) != 0 &&
        strncmp(buf, "quit", 4) != 0 )){
            isValid=1;
        }
        if(strncmp(buf, "Prices", 6) == 0 && size_of_argv==3) {
                //check the date format first
                if(checkDateFormat(getargv[2])!=0) isValid=1;
        }
        if(strncmp(buf, "MaxProfit", 9) == 0 && size_of_argv==4) {
            //check the date format first
            if(checkDateFormat(getargv[2])!=0 || checkDateFormat(getargv[3])!=0) isValid=1;
        }
        if(isValid==1){
            memset(buf, 0, sizeof(buf));
            fputs("Invalid syntax\n", stdout);
            continue;
        }
        // IF VALID: Send user input to the server
        write(clientfd, buf, strlen(buf));
        memset(buf, 0, sizeof(buf));
        // Receive the server's response
        n = read(clientfd, buf, MAXLINE);
        if (n == 0) {
            // The server closed the connection
            break;
        } else if (n < 0) {
            perror("Error reading from server");
            break;
        }
        // Display the server's response
        fputs(buf, stdout);
        //clear buf
        memset(buf, 0, sizeof(buf));
    }
}
int main(int argc, char **argv) {
    int clientfd;
    char *host, *port, buf[MAXLINE];
    host = argv[1];
    port = argv[2];
    clientfd= open_clientfd(host, port);
    client_echo(clientfd);
    close(clientfd);
    exit(0);
}