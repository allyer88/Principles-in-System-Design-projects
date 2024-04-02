#define _POSIX_C_SOURCE 201712L
#define MAXLINE 256
#define MAXARGS 256
typedef struct sockaddr SA;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <math.h>
struct FileMap
{
    char *name;
    FILE *file;
    // int fd;
};
struct FileMap fmap[2];
void initializefmap()
{
    for (int i = 0; i < 2; i++)
    {
        fmap[i].name = "";
        fmap[i].file = NULL;
    }
}
int open_listenfd(char *port)
{
    struct addrinfo hints, *listp, *p;
    int listenfd, optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;
    getaddrinfo(NULL, port, &hints, &listp);

    for (p = listp; p; p = p->ai_next)
    {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            continue;
        }

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            break;
        }

        close(listenfd);
    }

    freeaddrinfo(listp);
    if (!p)
    {
        return -1;
    }

    if (listen(listenfd, 1) < 0)
    {
        close(listenfd);
        return -1;
    }

    return listenfd;
}

void List(char *stockNames)
{
    if (fmap[0].name != "" && fmap[0].file != NULL)
    {
        int i = 0;
        while (i < strlen(fmap[0].name) && fmap[0].name[i] != '.')
        {
            strncat(stockNames, &fmap[0].name[i], 1);
            i++;
        }
    }

    if (fmap[1].name != "" && fmap[1].file != NULL)
    {
        if (*stockNames != '\0')
        {
            strcat(stockNames, " | ");
        }
        int i = 0;
        while (i < strlen(fmap[1].name) && fmap[1].name[i] != '.')
        {
            strncat(stockNames, &fmap[1].name[i], 1);
            i++;
        }
    }
    strcat(stockNames, "\n");
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
// Prices should be rounded up to the second digit after the decimal point
double roundUp(double number, int decimalPlaces)
{
    double multiplier = pow(10, decimalPlaces);
    return round(number * multiplier) / multiplier;
}

FILE* getFile(char *name){
    FILE *file=NULL;
    // Get which file to read
    if (strncmp(name, fmap[0].name, strlen(name)) == 0)
    {
        file = fmap[0].file;
    }
    else if (strncmp(name, fmap[1].name, strlen(name)) == 0)
    {
        file = fmap[1].file;
    }
    return file;
}
double Prices(char *name, char *date)
{
    double price = -1.0;
    char line[MAXLINE];
    char *getargv[MAXARGS];
    FILE* file = getFile(name);
    if(file==NULL){
        printf("Can not find the file\n");
        return -1.0;
    }
    // Read lines until find the date
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (strncmp(date, line, 10) != 0)
        {
            continue;
        }
        else
        {
            int size_of_argv = parseline(line, getargv);
            price = roundUp(atof(getargv[4]), 2);
            break;
        }
    }
    /// return to the start of the file
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        perror("Error seeking to the start of the file");
        fclose(file);
    }
    return price;
}
//increase the prices array if over the size
void increaseSize(double** prices, int newSpace) {
    double *temp = realloc(*prices, newSpace * sizeof(double));
    if (temp == NULL) {
        perror("Memory reallocation failed");
        // Handle the error, maybe return or exit the program
    } else {
        *prices = temp; // Update the original pointer outside the function
    }
}
double findMaxProfit(double* prices, int size){
    double minPrice;
    double maxProfit= 0 ;
    for(int i=0;i<size;i++){
        minPrice=prices[i];
        for(int j=i+1; j<size;j++){
            if(prices[j]>minPrice){
                if(prices[j]-minPrice>maxProfit) maxProfit= prices[j]-minPrice;
            }
        }
    }
    return roundUp(maxProfit,2);
}
double MaxProfit(char *name, char *startDate, char *endDate){
    char line[MAXLINE];
    char *getargv[MAXARGS];
    FILE* file = getFile(name);
    if(file==NULL){
        printf("Can not find the file\n");
        return -1.0;
    }
    //store all price in a array
    int array_space = 30;
    int array_size=0;
    double* prices = malloc(array_space * sizeof(double));
    // Read lines until find the start date
    //test to make sure the date is enter correctly
    int test=0;
    //store all prices into an array
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (strncmp(startDate, line, 10) != 0)
        {
            continue;
        }
        else
        {
            parseline(line, getargv);
            prices[array_size] = atof(getargv[4]);
            array_size++;
            test++;
            break;
        }
    }
    //if not find start date in file, return
    if(test==0) {
        if (fseek(file, 0, SEEK_SET) != 0)
        {
            perror("Error seeking to the start of the file");
            fclose(file);
        }
        free(prices);
        return -1.0;
    }
    //find till the end date
    test=0;
    while (fgets(line, sizeof(line), file) != NULL)
        {
    
            if (strncmp(endDate, line, 10) != 0)
            {
                parseline(line, getargv);
                if(array_size==array_space){
                    array_space=2*array_space;
                    increaseSize(&prices, array_space);
                }
                prices[array_size] = atof(getargv[4]);
                array_size++;
            }
            else
            {
                parseline(line, getargv);
                if(array_size==array_space){
                    array_space=2*array_space;
                    increaseSize(&prices, array_space);
                }
                prices[array_size] = atof(getargv[4]);
                array_size++;
                test++;
                break;
            }
            
        }
    if(test==0) {
        if (fseek(file, 0, SEEK_SET) != 0)
        {
            perror("Error seeking to the start of the file");
            fclose(file);
        }
        free(prices);
        return -1.0;
    }
    double maxProfit = findMaxProfit(prices, array_size);
    /// return to the start of the file
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        perror("Error seeking to the start of the file");
        fclose(file);
    }
    free(prices);
    return maxProfit;
}
void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    while ((n = read(connfd, buf, MAXLINE)) != 0)
    {
        if(strncmp(buf, "quit", 4) != 0) fputs(buf, stdout);
        if (n >= 4 && strncmp(buf, "List", 4) == 0)
        {
            char stockName[MAXLINE] = "";
            List(stockName);
            write(connfd, stockName, strlen(stockName));
        }
        else if ((n >= 6 && strncmp(buf, "Prices", 6) == 0)||(n >= 9 && strncmp(buf, "MaxProfit", 9) == 0))
        {
            char *getargv[MAXARGS];
            int size_of_argv = parseline(buf, getargv);
            double price;
            if(strncmp(buf, "Prices", 6) == 0) {
                price = Prices(getargv[1], getargv[2]);
            }
            else if(strncmp(buf, "MaxProfit", 9) == 0) {
               price = MaxProfit(getargv[1],getargv[2],getargv[3]);
            }
            char str[20];
            sprintf(str, "%.2f\n", price);
            if (price == -1.0)
                {
                    write(connfd, "Unknown\n", 8);
                }
                else
                {
                    write(connfd, str, strlen(str));
                }

        }
        else if (n >= 4 && strncmp(buf, "quit", 4) == 0)
        {
            fclose(fmap[0].file);
            if(fmap[1].file!=NULL) fclose(fmap[1].file);
            close(connfd);
            exit(0);
        }
        else
        {
            write(connfd, "Invalid syntax\n", 15);
        }
        memset(buf, 0, sizeof(buf));
    }
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
        // initialize fmap
    initializefmap();
    FILE *file1 = fopen(argv[1], "r");
    //save to fmap
    if (file1 != NULL)
    {
        fmap[0].name = argv[1];
        fmap[0].file = file1;
    }
    FILE *file2;
    //if only one file passes
    if(argc==4){
        file2 = fopen(argv[2], "r");
        //save to fmap
        if (file1 != NULL)
        {
            if (fmap[0].name != "")
            {
                fmap[1].name = argv[2];
                fmap[1].file = file2;
            }
            else
            {
                fmap[0].name = argv[2];
                fmap[0].file = file2;
            }
        } 
    }
    
    if(argc==4)listenfd = open_listenfd(argv[3]);
    else if(argc==3)listenfd = open_listenfd(argv[2]);
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        getnameinfo((SA *)&clientaddr, clientlen,
                    client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        printf("server started\n");
        echo(connfd);
        fclose(file1);
        if(argc==4) fclose(file2);
        close(connfd);
    }
    exit(0);
}