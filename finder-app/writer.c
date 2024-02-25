#include <syslog.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    openlog("Logs", LOG_PID, LOG_USER);
    if (argc != 3)
    {
        printf("This script expects two arguments.");
        syslog(LOG_ERR, "Insufficient arguments.");
        return 1;
    }
    char *filename = argv[1];

    // open the file for writing
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Error opening the file %s", filename);
        syslog(LOG_ERR, "Error opening the file %s", argv[1]);

        return -1;
    }
    fprintf(fp, "%s", argv[2]);
    syslog(LOG_DEBUG, "Writing %s{0} to %s{1}", argv[1], argv[2]);

    // close the file
    fclose(fp);
    closelog();

    return 0;
}