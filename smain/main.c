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


int connecttoserver(char* filename, char* servername) {
    int server;
    char* port = strcmp(servername, "pdf") == 0? "9533": "9534";
    char* ipaddr = "127.0.0.1";
    int portNumber;
    struct sockaddr_in servAdd;

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) { //socket()
        fprintf(stderr, "Cannot create socket\n");
        exit(1);
    }

    sscanf(port, "%d", &portNumber);

    servAdd.sin_family = AF_INET; //Internet 
    servAdd.sin_port = htons((uint16_t) portNumber); //Port number

    if (inet_pton(AF_INET, ipaddr, &servAdd.sin_addr) < 0) {
        fprintf(stderr, " inet_pton() has failed\n");
        exit(2);
    }

    if (connect(server, (struct sockaddr* ) &servAdd, sizeof(servAdd)) < 0) { //Connect()
        fprintf(stderr, "connect() failed, exiting\n");
        perror("connect failed");
        exit(3);
    }

    char*  datatoserver = "This is test data sent to server";

    write(server, datatoserver, strlen(datatoserver));

    exit(0);
}

int handlecommand(char* userinput, int client) {
    char cmd[100];
    char filename[100];
    char dest[100];
    char* mainfolder = "~smain";
    char* textfolder = "~stext";
    char* pdffolder = "~spdf";
    char destpath[100];
    sscanf(userinput, "%s %s %s", cmd, filename, dest);
    printf("%s, %s, %s\n", cmd, filename, dest);

    // ufile command
    if (strcmp(cmd, "ufile") == 0) {

        if(strncmp(dest, mainfolder, strlen(mainfolder)) == 0) {
            strcpy(destpath, dest + strlen(mainfolder) + 1);

            struct stat st;
            // Check if the directory exists
            if (stat(destpath, &st) != 0 || !S_ISDIR(st.st_mode)) {
                // Directory does not exist, attempt to create it

                char newpath[100] = "";
                int count = 0;
                char temppath[100];
                strcpy(temppath, destpath);
                for (char* separator = strtok(temppath, "/"); separator != NULL; separator = strtok(NULL, "/"), count+=1) {
                    if(count!=0)
                    strcat(newpath, "/");
                    strcat(newpath, separator);
                    if(stat(newpath, &st) != 0 || !S_ISDIR(st.st_mode)) {
                        if (mkdir(newpath, 0700) != 0) {
                            perror("mkdir failed");
                            return -1; // Failed to create directory
                        }
                    }

                }
            }
        }

        int filesize;
        char filesizebuf[100];
        // get size of file
        int recbytes = read(client, filesizebuf, 100);
        if (recbytes < 0) {
            printf("\nError receiving filesize\n");
            return 1;
        }

        filesize = atoi(filesizebuf);
        strcat(destpath, "/");
        strcat(destpath, filename);

        int fd = open(destpath, O_CREAT | O_RDWR | O_APPEND, 0777);
        if(fd < -1) {
            printf("\nError opening file\n");
            return 1;
        }
        int totalbytesread = 0;
        char filebuf[100];

        char* message="received";
        send(client, message, strlen(message), 0);

        while(totalbytesread < filesize) {
            // int bytesread = read(client, filebuf, 100);
            int bytesread = recv(client, filebuf, 100, 0);
            totalbytesread += bytesread;
            int writebytes = write(fd, filebuf, bytesread);
            if(writebytes < 0) {
                printf("\nError writing file\n");
                return 1;
            }
        }

        printf("\nDone\n");
        
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
    char buff1[1024];
    int bytes_read = read(client, buff1, 1024);
    if (bytes_read < 0) {
        printf("Server: read() failure\n");
        exit(3);
    }

    handlecommand(buff1, client);


    // to test pdf server connection
    // if (strcmp(buff1, "pdf") == 0) {
    //     connecttoserver("test", "pdf");
    // }
    close(client);
    printf("\n");
    exit(0);
} else if (pid > 0) {
    close(client);
}
}

}
