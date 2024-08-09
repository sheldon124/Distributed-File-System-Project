#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h> 
#include <unistd.h>
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
    char destpath[50];
    sscanf(userinput, "%s %s %s", cmd, filename, dest);
    printf("%s, %s, %s\n", cmd, filename, dest);

    // ufile command
    if (strcmp(cmd, "ufile") == 0) {
        printf("Here\n");

        if(strncmp(dest, mainfolder, strlen(mainfolder)) == 0) {
            strcpy(destpath, dest + strlen(mainfolder) + 1);
            printf("\nNew Path: %s\n", destpath);

            struct stat st;
            // Check if the directory exists
            if (stat(destpath, &st) != 0 || !S_ISDIR(st.st_mode)) {
                // Directory does not exist, attempt to create it
                printf("Directory no exist\n");

                char newpath[100] = "";
                int count = 0;
                for (char* separator = strtok(destpath, "/"); separator != NULL; separator = strtok(NULL, "/"), count+=1) {
                    if(count!=0)
                    strcat(newpath, "/");
                    strcat(newpath, separator);
                    printf("%s %s\n", separator, newpath);
                    if(stat(newpath, &st) != 0 || !S_ISDIR(st.st_mode)) {
                        if (mkdir(newpath, 0700) != 0) {
                            perror("mkdir failed");
                            return -1; // Failed to create directory
                        }
                    }

                }

                printf("newpth: %s", newpath);
            }
        }
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
