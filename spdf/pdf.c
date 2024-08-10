#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h> 
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


int ufilecommand(char *cmd, char *filename, char *dest, int client) {
    int pathprovided = 1;
    char destpath[100];
    struct stat st;

    if (strcmp(dest, "/") == 0) {
        pathprovided = 0;
        strcpy(destpath, "");
    }
    else
        strcpy(destpath, dest);

    if (pathprovided && (stat(destpath, &st) != 0 || !S_ISDIR(st.st_mode))) {
        // Directory does not exist, attempt to create it
        char newpath[100] = "";
        int count = 0;
        char temppath[100];
        strcpy(temppath, destpath);
        for (char *separator = strtok(temppath, "/"); separator != NULL; separator = strtok(NULL, "/"), count += 1) {
            if (count != 0)
                strcat(newpath, "/");
            strcat(newpath, separator);
            if (stat(newpath, &st) != 0 || !S_ISDIR(st.st_mode)) {
                if (mkdir(newpath, 0700) != 0) {
                    perror("mkdir failed");
                    return -1; // Failed to create directory
                }
            }
        }
    }

    int filesize;
    char filesizebuf[100];
    // get size of file
    int recbytes = read(client, filesizebuf, 100);
    if (recbytes < 0)
    {
        printf("\nError receiving filesize\n");
        return 1;
    }

    filesize = atoi(filesizebuf);

    if (pathprovided)
    {
        strcat(destpath, "/");
        strcat(destpath, filename);
    }
    else
        strcpy(destpath, filename);

    int fd = open(destpath, O_CREAT | O_RDWR | O_APPEND, 0777);
    if (fd < -1)
    {
        printf("\nError opening file\n");
        return 1;
    }

    char *message = "received";
    send(client, message, strlen(message), 0);

    int totalbytesread = 0;
    char filebuf[100];

    while (totalbytesread < filesize)
    {
        // int bytesread = read(client, filebuf, 100);
        int bytesread = recv(client, filebuf, 100, 0);
        totalbytesread += bytesread;
        int writebytes = write(fd, filebuf, bytesread);
        if (writebytes < 0)
        {
            printf("\nError writing file\n");
            return 1;
        }
    }

    printf("\nDone\n");

    return 0;
}

int prcclient(char* inputcommand, int client) {
    char cmd[100];
    char filename[100];
    char dest[100];
    sscanf(inputcommand, "%s %s %s", cmd, filename, dest);
    if(strcmp(cmd, "ufile") == 0) {
        return ufilecommand(cmd, filename, dest, client);
    }
    return 0;
}
  
int main(int argc, char *argv[]){//E.g., 1, server
char *myTime;
time_t currentUnixTime; // time.h
int sd, client, portNumber;
socklen_t len;
struct sockaddr_in servAdd;

if(argc != 2){
fprintf(stderr,"Call model: %s <Port#>\n",argv[0]);
exit(0);
}
//socket()
if((sd = socket(AF_INET, SOCK_STREAM, 0))<0){
fprintf(stderr, "Could not create socket\n");
exit(1);
}

servAdd.sin_family = AF_INET;
servAdd.sin_addr.s_addr = htonl(INADDR_ANY);
sscanf(argv[1], "%d", &portNumber);
servAdd.sin_port = htons((uint16_t)portNumber);

//bind
bind(sd, (struct sockaddr *) &servAdd,sizeof(servAdd));

if (listen(sd, 5) < 0) {
    perror("listen failed");
    exit(1);
}

while(1) {
    
    client=accept(sd,(struct sockaddr*)NULL,NULL);//accept()
    if(client < 0) {
        perror("Error accepting connection");
        continue;
    }

    int pid = fork();
    
    if (pid == 0) {
    char buff1[100];
    int bytes_read = read(client, buff1, 100);
    if (bytes_read < 0) {
        printf("Server: read() failure\n");
        exit(3);
    }
    printf("%s\n", buff1);
    prcclient(buff1, client);
    close(client);
    printf("\n");
    exit(0);
} else if (pid > 0) {
    close(client);
}
}

}
