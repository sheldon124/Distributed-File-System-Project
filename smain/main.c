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
#include <dirent.h>

int connecttoserver(char* command, char* servername) {
    int server;
    char* port = strcmp(servername, "pdf") == 0? "9533": "9534";
    char* ipaddr = "127.0.0.1";
    int portNumber;
    struct sockaddr_in servAdd;

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) { //socket()
        fprintf(stderr, "Cannot create socket\n");
        return -1;
    }

    sscanf(port, "%d", &portNumber);

    servAdd.sin_family = AF_INET; //Internet 
    servAdd.sin_port = htons((uint16_t) portNumber); //Port number

    if (inet_pton(AF_INET, ipaddr, &servAdd.sin_addr) < 0) {
        fprintf(stderr, " inet_pton() has failed\n");
        return -1;
    }

    if (connect(server, (struct sockaddr* ) &servAdd, sizeof(servAdd)) < 0) { //Connect()
        fprintf(stderr, "connect() failed, exiting\n");
        perror("connect failed");
        return -1;
    }
    

    write(server, command, strlen(command));

    return server;
}

int getfilesfromserver(char* command, char* servername, int client) {
    int server = connecttoserver(command, servername);
    if (server < 0) {
        printf("\nError establishing connection to text server\n");
        return -1;
    }

    char filename[100];
    int sizereceived;
    while(((sizereceived = recv(server, filename, 100, 0))) > 0) {
        filename[sizereceived] = '\0';
        if(strcmp(filename, "complete") == 0) {
            break;
        } 
        else {
            int n = send(client, filename, strlen(filename), 0);
            if(n < 0) {
                printf("Display to client failed\n");
                return -1;
            }
        }
    }

    close(server);
    return 0;
}


int listfiles(char* userpath, int client) {

    char path[100];
    char *mainfolder = "~smain";
    int pathprovided = 1;
    if (strcmp(userpath, mainfolder) == 0)
    {
        pathprovided=0;
        strcpy(path, "./");
    }
    else
        strcpy(path, userpath + strlen(mainfolder) + 1);
    struct stat st;
    if(stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("\nDirectory path does not exist in smain");
    }
    else {
        DIR* directory;
        if (!(directory = opendir(path))) {
            perror("Error opening directory");
            return 1;
        }
        for(struct dirent *direntry = readdir(directory); direntry != NULL; direntry = readdir(directory)) {

            if (strcmp(direntry->d_name, ".") == 0 || strcmp(direntry->d_name, "..") == 0) {
            continue;
        }

            char new_path[1024];
            snprintf(new_path, sizeof(new_path), "%s/%s", path, direntry->d_name);

        if (direntry->d_type == DT_REG) {
            printf("File: %s\n", direntry->d_name);
            int n = send(client, direntry->d_name, strlen(direntry->d_name), 0);
            if(n < 0) {
                printf("Display to client failed\n");
                return 1;
            }
            sleep(0.5);
        }
        // else if (direntry->d_type == DT_DIR) {
        //     listfiles(new_path);
        // }
        }
        closedir(directory);
    }

    char command[100];

    strcpy(command, "display");
    strcat(command, " ");
    strcat(command, path);
    command[strlen(command)] = '\0';

    int serverstatus = getfilesfromserver(command, "pdf", client);
    if(serverstatus < 0) {
        printf("\nError getting files from Text Server\n");
    }
    serverstatus = getfilesfromserver(command, "text", client);
    if(serverstatus < 0) {
        printf("\nError getting files from Text Server\n");
    }


    char complete[50] = "complete";
    complete[strlen(complete)] = '\0';
    int n = send(client, complete, strlen(complete), 0);
    if (n < 0)
    {
        printf("Display to client failed\n");
        return 1;
    }
    sleep(0.5);

    return 0;

}

int uploadtoserver(int client, int server) {
    int filesize;
    char filesizebuf[100];
    // get size of file
    int recbytes = read(client, filesizebuf, 100);
    if (recbytes < 0) {
        printf("\nError receiving filesize\n");
        return 1;
    }

    filesize = atoi(filesizebuf);

    int n;

    // n= write(socket, leninstr, strlen(leninstr));
    n = send(server, filesizebuf, strlen(filesizebuf), 0);

    if (n < 0) {
        printf("\nSend filesize to text server failed\n");
        return 1;
    }

    char confirmation[8];
    int recvconfirm = recv(server, confirmation, 8, 0);
    if (recvconfirm < 0) {
        printf("\nError uploading file\n");
        return 1;
    }

    char* message = "received";
    send(client, message, strlen(message), 0);

    printf("\nTransferring to text\n");

    int totalbytesread = 0;
    char filebuf[100];
    while (totalbytesread < filesize) {
        // int bytesread = read(client, filebuf, 100);
        int bytesread = recv(client, filebuf, 100, 0);
        totalbytesread += bytesread;
        int n = send(server, filebuf, bytesread, 0);
        if (n < 0) {
            printf("\nWrite Failed\n");
        }
    }

    return 0;
}

int uploadtomain(int client, char* destpath, int pathprovided, char* filename) {
    struct stat st;
    // Check if the directory exists
    if (pathprovided && (stat(destpath, &st) != 0 || !S_ISDIR(st.st_mode))) {
        // Directory does not exist, attempt to create it
        char newpath[100] = "";
        int count = 0;
        char temppath[100];
        strcpy(temppath, destpath);
        for (char* separator = strtok(temppath, "/"); separator != NULL; separator = strtok(NULL, "/"), count += 1) {
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
    if (recbytes < 0) {
        printf("\nError receiving filesize\n");
        return 1;
    }

    filesize = atoi(filesizebuf);
    if (pathprovided) {
        strcat(destpath, "/");
        strcat(destpath, filename);
    } else
        strcpy(destpath, filename);

    int fd = open(destpath, O_CREAT | O_RDWR | O_APPEND, 0777);
    if (fd < -1) {
        printf("\nError opening file\n");
        return 1;
    }
    int totalbytesread = 0;
    char filebuf[100];

    char* message = "received";
    send(client, message, strlen(message), 0);

    while (totalbytesread < filesize) {
        // int bytesread = read(client, filebuf, 100);
        int bytesread = recv(client, filebuf, 100, 0);
        totalbytesread += bytesread;
        int writebytes = write(fd, filebuf, bytesread);
        if (writebytes < 0) {
            printf("\nError writing file\n");
            return 1;
        }
    }

    return 0;
}

int ufilecommand(char* cmd, char* filename, char* dest, int client) {
    char* mainfolder = "~smain";
    char servername[5];
    char destpath[100];
    if (strncmp(dest, mainfolder, strlen(mainfolder)) == 0) {
        int pathprovided = 1;
        if (strcmp(dest, mainfolder) == 0) {
            pathprovided = 0;
            strcpy(destpath, "");
        } else
            strcpy(destpath, dest + strlen(mainfolder) + 1);

        struct stat st;

        int server;
        char ext[5];
        char* dot = strrchr(filename, '.');
        if (!dot || dot == filename) {
            strcpy(ext, "");
        } else strcpy(ext, dot + 1);

        if (strcmp(ext, "txt") == 0) {
            strcpy(servername, "text");

            char command[100];

            strcpy(command, cmd);
            strcat(command, " ");
            strcat(command, filename);
            strcat(command, " ");
            if (pathprovided)
                strcat(command, destpath);
            else
                strcat(command, "/");


            printf("\nText Command %s\n", command);

            server = connecttoserver(command, servername);

            if(server < 0) {
                printf("\nError establishing connection to text server\n");
                return 1;
            }

            int serverstatus = uploadtoserver(client, server);

            close(server);
            printf("\nDone\n");
            return serverstatus;
        } else if (strcmp(ext, "pdf") == 0) {
            strcpy(servername, "pdf");
             char command[100];

            strcpy(command, cmd);
            strcat(command, " ");
            strcat(command, filename);
            strcat(command, " ");
            if (pathprovided)
                strcat(command, destpath);
            else
                strcat(command, "/");


            printf("\nPDF Command %s\n", command);

            server = connecttoserver(command, servername);

            if(server < 0) {
                printf("\nError establishing connection to text server\n");
                return 1;
            }

            int serverstatus = uploadtoserver(client, server);

            close(server);
            printf("\nDone\n");
            return serverstatus;
        } else if (strcpy(servername, "c")) {
            strcpy(servername, "main");
        } else {
            printf("\nInvalid file type\n");
            return 1;
        }

        if (strcmp(servername, "main") == 0) {
            int uploadstatus = uploadtomain(client, destpath, pathprovided, filename);
            printf("\nDone\n");
            return uploadstatus;
        }
    }
}

int handlecommand(char* userinput, int client) {
    char cmd[100];
    char filename[100];
    char dest[100];
    char* mainfolder = "~smain";
    char destpath[100];
    sscanf(userinput, "%s %s %s", cmd, filename, dest);
    printf("%s, %s, %s\n", cmd, filename, dest);

    // ufile command
    if (strcmp(cmd, "ufile") == 0) {
        return ufilecommand(cmd, filename, dest, client);
    }
    else if(strcmp(cmd, "display") == 0) {
        return listfiles(filename, client);
    }
    return 0;
}
  
int main(int argc, char *argv[]){

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

    int success = handlecommand(buff1, client);

    if(success == 0) {
        char* message = "Successful";
        send(client, message, strlen(message), 0);
    }
    else {
        char* message = "Failed";
        send(client, message, strlen(message), 0);
    }


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
