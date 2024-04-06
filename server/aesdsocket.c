#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/stat.h>
#define BUFFER_SIZE 32000
char *buffer;

void error(const char *msg)
{
    //
    //printf("FREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
    //free(buffer);
    perror(msg);
    exit(-1);
}

ssize_t read_line(int fileDesc, void *buffer, size_t n)
{
    ssize_t numberOFReads;
    size_t total = 0;
    
    char ch;

    if (n <= 0 || buffer == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    char *buf = buffer;
    total = 0;
    while (1)
    {
        numberOFReads = read(fileDesc, &ch, 1);
        if (numberOFReads == -1)
        {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }
        else if (numberOFReads == 0)
        {
            if (total == 0)
                return total;
            else
                break;
        }
        else
        {
            if (total < n - 1)
            {
                *buf++ = ch;
                total++;
            }
            if (ch == '\n')
                break;
        }
    }

    *buf = '\0';
    return total;
}

////////////////////////////////////////////////////////
  

int bind_or_connect(char *host, char *port,
                    int _bind)
{
    struct addrinfo *result;

    struct addrinfo myaddrinf;

    memset(&myaddrinf, 0, sizeof(struct addrinfo));
    myaddrinf.ai_family = AF_UNSPEC;
    myaddrinf.ai_socktype = SOCK_STREAM;
    myaddrinf.ai_flags = AI_PASSIVE;
    myaddrinf.ai_canonname = NULL;
    myaddrinf.ai_addr = NULL;
    myaddrinf.ai_next = NULL;

    if (getaddrinfo(host, port, &myaddrinf,
                    &result) != 0)
        error("getAddressInfo: getaddrinfo failed.");

    struct addrinfo *addr2;
    int outcode;
    for (addr2 = result; addr2 != NULL; addr2 = addr2->ai_next)
    {
        outcode = socket(addr2->ai_family, addr2->ai_socktype,
                     addr2->ai_protocol);

        if (outcode == -1)
            continue;

        int err;
        if (_bind)
            err = bind(outcode, addr2->ai_addr,
                       addr2->ai_addrlen);
        else
            err = connect(outcode, addr2->ai_addr,
                          addr2->ai_addrlen);
        if (!err)
            break;
    }

    if (addr2 == NULL)
    {
        syslog(LOG_ERR, "server: Failed to bind.\n");
        exit(EXIT_FAILURE);
    }
 
    freeaddrinfo(result);

    return outcode;
}

////////////////////////////////////////////////////

char *filename = "/var/tmp/aesdsocketdata";

int main(int argc, char *argv[])
{
    int sockfd, newsockfd;
    char *portno;
    FILE *filePointer;
    socklen_t clilen;
    int fdesc;
    struct sockaddr cli_addr;
    int n;
    char *buffer = (char *)malloc(BUFFER_SIZE);
    if (buffer == NULL)
        error("memory allocation for buffer failed.");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    if (argc > 2)
    {
        portno = argv[2];
    }
    else
    {
        portno = "9000";
    }
    int pid;
    if (argc > 1)
    {
        if (strcmp(argv[1], "-d") == 0)
        {
            pid = fork();
            if ( pid > 0 )
            {
                syslog(LOG_INFO, "Successfully forked: %d", pid);
                exit(EXIT_SUCCESS);
            }
            else if (pid == -1)
            {
                syslog(LOG_ERR, "Failed to fork(): %s", strerror(errno));
                error("ERROR forking.");
            } 

            if (setsid() == -1)
            { 
                error("ERROR forking Failed to create new session and group");
            }

            if (chdir("/") == -1)
            {
                error("ERROR forking Failed to changed to root dir"); 
            }

            open("/dev/null", O_RDWR);
            dup(0);
            dup(1);
        }
    }

    // printf("=========================portchar is: %s",portchar);
    sockfd = bind_or_connect((char *)"localhost", portno, 1);

    if (listen(sockfd, 5) == -1)
        error("could not listen on port.");
    clilen = sizeof(cli_addr);
    int count = 0;
    int number_of_reads = 0;

    fdesc = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fdesc == -1)
    {
        error("could not truncate file.");
    }
    close(fdesc);

    while (1)
    {
        newsockfd = accept(sockfd,
                           (struct sockaddr *)&cli_addr,
                           &clilen);
        if (newsockfd == -1)
            error("ERROR on accept");
        bzero(buffer, BUFFER_SIZE);
        n = read_line(newsockfd, buffer, BUFFER_SIZE);
        if (n == -1)
            error("ERROR reading from socket");

        count = strlen(buffer);
        fdesc = open(filename, O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
        if (fdesc == -1)
        {
            error("could not open file");
        }
        syslog(LOG_INFO, "Opened file %s", filename);

        number_of_reads = write(fdesc, buffer, count);
        if (number_of_reads == -1)
            error("could not read file");
        else if (number_of_reads != count)
            error("could not write to file");
        syslog(LOG_INFO, "Wrote msg to %s", filename);
        close(fdesc);

        filePointer = fopen(filename, "r");

        memset(buffer, 0, BUFFER_SIZE);
        count = 0;
        while (fgets(buffer, BUFFER_SIZE, filePointer) != NULL)
        {
            send(newsockfd, buffer, strlen(buffer), 0);
        }
        fclose(filePointer);
    }
    free(buffer);
    close(newsockfd);
    close(sockfd);
    return 0;
}
