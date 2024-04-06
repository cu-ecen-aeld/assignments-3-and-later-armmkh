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
char *buffer ;

void error(const char *msg)
{
    //
    printf("FREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
    free(buffer);
    perror(msg);
    exit(-1);
}

ssize_t read_line(int fdesc, void *buffer, size_t n)
{
    if (n < 1 || buffer == NULL)
        return -1;
    char c;

    size_t total = 0;
    char *buffer2 = buffer;
    long readBytes = 0;
    while (1)
    {
        readBytes = read(fdesc, &c, 1);

        switch (readBytes)
        {
        case -1:
            if (errno == EINTR)
                continue;
            else
                return -1;
            break;
        case 0:
            if (total == 0)
                return total; // no bytes to read
            else
                break;

        default:
            if (total < n - 1)
            {
                total++;
                *buffer2++ = c;
            }

            if (c == '\n')
                break;
        }
    }
    return total;
}

////////////////////////////////////////////////////////
 
typedef struct NameInformation
{
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
} NameInformation;
void get_name_info(struct sockaddr *ai_addr, socklen_t ai_addrlen,
                   NameInformation *nameInfo)
{
    int s;
    s = getnameinfo(ai_addr, ai_addrlen,
                    nameInfo->host, NI_MAXHOST,
                    nameInfo->serv, NI_MAXSERV, 0);
    if (s != 0)
    {
        syslog(LOG_ERR, "_getNameInfo: getnameinfo failed: %s\n", gai_strerror(s));
        syslog(LOG_ERR, "sa_family: %d\n sa_data: %s\n", ai_addr->sa_family,
               ai_addr->sa_data);
        exit(EXIT_FAILURE);
    }
}

int getSocket(int _bind, struct addrinfo *result)
{
    struct addrinfo *rp;
    int sfd;
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);

        if (sfd == -1)
            continue;

        int err;
        if (_bind)
            err = bind(sfd, rp->ai_addr,
                       rp->ai_addrlen);
        else
            err = connect(sfd, rp->ai_addr,
                          rp->ai_addrlen);
        if (!err)
            break;
    }

    if (rp == NULL)
    {
        syslog(LOG_ERR, "server: Failed to bind.\n");
        exit(EXIT_FAILURE);
    }

    return sfd;
}

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

    int x = getSocket(_bind, result);
    freeaddrinfo(result);

    return x;
}

////////////////////////////////////////////////////

char *filename = "/var/tmp/aesdsocketdata";

int main(int argc, char *argv[])
{
    int sockfd, newsockfd;
    int portno = 9000;
    FILE *filePointer;
    socklen_t clilen;
    int fdesc;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    char *buffer = (char *)malloc(BUFFER_SIZE);
    if (buffer == NULL)
        error("memory allocation for buffer failed.");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
 
    char portchar[4];
    sprintf(portchar, "%d", portno);

    //printf("=========================portchar is: %s",portchar);
    sockfd = bind_or_connect((char *)"localhost", portchar, 1);

    int pid;
    if (argc > 1)
    {
        if (strcmp(argv[1], "-d") == 0)
        {
            pid = fork();
            if (pid == -1)
            {
                syslog(LOG_ERR, "Failed to fork(): %s", strerror(errno));
                error("ERROR forking");
            }
            else if (pid != 0)
            {
                syslog(LOG_INFO, "Successfully forked: %d", pid);
                error("ERROR forking");
            }

            if (setsid() == -1)
            {
                syslog(LOG_ERR, "Failed to create new session and group: %s",
                       strerror(errno));
                error("ERROR forking");
            }

            if (chdir("/") == -1)
            {
                syslog(LOG_ERR, "Failed to changed to root dir: %s",
                       strerror(errno));
                error("ERROR forking");
            }

            open("/dev/null", O_RDWR);
            dup(0);
            dup(1);
        }
    }
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
        if (newsockfd < 0)
            error("ERROR on accept");
        bzero(buffer, BUFFER_SIZE);
        n = read_line(newsockfd, buffer, BUFFER_SIZE);
        if (n < 0)
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
