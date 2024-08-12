#define _GNU_SOURCE  // Enable GNU extensions, including strsep
#define _XOPEN_SOURCE 500  // Enable POSIX 1995 features, including nftw
#include <stdio.h> //printf
#include <stdlib.h> //EXIT_fAILURE EXIT_SUCCESS
#include <ftw.h> //ftw()
#include <sys/types.h> // Data types used in system calls.
#include <sys/stat.h> //For stat(), S_ISDIR() Macro
#include <unistd.h>  //Access to the POSIX operating system API.
#include <string.h> //For string methods
#include <stdint.h> // Include for int64_t
#include <limits.h> //For PATH_MAX
#include <fcntl.h>
#include <errno.h> //For errno
#include <stdbool.h> //boolean
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <regex.h>
#include <time.h>
#include <dirent.h>

//Shane Change
const char *baseDir = "/home/dsouza56/project/";
bool cFilesExist = false;

//Shane Change
void parseInput(char *input, char **commandArgv, int *commandArgc) {
    int index = 0;
    char *currentCmd;
    //Iterates and seperate the commands based on " " identified
    while ((currentCmd = strsep(&input, " ")) != NULL) { 
        if (*currentCmd != '\0') { //Empty check
            commandArgv[index] = currentCmd; //Adds cCarrent cmmand in argv
            //printf("Arg Index---> %s\n", argv[index]);
            index++;
        }
    }

    commandArgv[index] = NULL; // Addin Null terminate the last index is necessaru
    *commandArgc = index; //Assigning the value of index as count ref
}

//Shane Change
bool checkIfFileExists(const char *filepath){
    //return (access(filepath, F_OK) == 0) ? true : false; 
    struct stat fileInfo;
    return (stat(filepath, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) ? true : false;
}

//Shane Change
char *constructFullPath(const char *baseDir, const char *relativePath){
    static char fullPath[200];
    strcpy(fullPath, baseDir);
    // if (relativePath[0] == '/') {
    //     strcat(fullPath, relativePath + 1);  // Skip leading slash
    // } else {
    //     strcat(fullPath, relativePath);
    // }
    strcat(fullPath, relativePath + 1);
    return fullPath;
}

//Shane Change
const char* getFileExtension(const char *fullPath) {
    const char *pathExt = NULL;
    for(int i = strlen(fullPath) - 1; i >= 0; i--) {
        if(fullPath[i] == '.') { // Find the last dot in the path
            pathExt = &fullPath[i];
            break; // Exit the loop once the extension is found
        }
    }
    return pathExt;
}

//Shane Change
void downloadCFiles(const char *fullPath, int client){
    //First check if file exists in the directory
    if(!checkIfFileExists(fullPath)){
        printf("File does not exist!\n");
        char *msg = "File does not exist on the server.";
        write(client, msg, strlen(msg) + 1);
        return;
    }else{
        printf("File Exists\n");

        char fileBuffer[1024];
        long int bytesRead;
        long int bytesSent;

        //Reading File
        // Opens File Descriptor in Read Only from the source file
        int fdSrc = open(fullPath, O_RDONLY); 
        if (fdSrc == -1) { // If can not open
            printf("Error whille opening the file.\n");
            char *msg = "File not found on server.";
            write(client, msg, strlen(msg) + 1); // Send Error message to client
            close(fdSrc);
            return;
        }

        //File transfer indicator
        write(client, "dfile", strlen("dfile"));

        //Reading file
        // bytesRead = read(fdSrc, fileBuffer, 1024);
        // if (bytesRead == -1) { // If cannot read the file
        //     printf("Error in reading file '%s'\n", fdSrc);        
        // } 
        // printf("Total Bytes Read %d\n", bytesRead);

        //Reading in chunks
        while( ( bytesRead = read(fdSrc, fileBuffer, 1024) ) > 0 ){
            bytesSent = send(client, fileBuffer, bytesRead, 0);
            if (bytesSent < 0) { 
                printf("Error in sending file\n");
                close(fdSrc);
                return;    
            } 
        }

        //Read err
        if (bytesRead < 0) {
            printf("Error reading the file");
        }
        close(fdSrc);
        printf("File sent from Server.\n");
    }
}

//Shane Change
void downloadHandler(char **commandArgv, int commandArgc, int client){
    
    const char *fullPath = constructFullPath(baseDir, commandArgv[1]);
    //printf("FullPath: %s\n", fullPath);

    const char *pathExt = getFileExtension(fullPath);
    //printf("Path Ext: %s\n", pathExt);

    //Processing for .c
    if (strcmp(pathExt, ".c") == 0) {
        printf("Process for .c\n");
        downloadCFiles(fullPath, client);
    } else if (strcmp(pathExt, ".txt") == 0) {
        printf("Process for .txt\n");
    } else if (strcmp(pathExt, ".pdf") == 0) {
        printf("Process for .pdf\n");
    }
    
}

//Shane Change
void removeCFiles(const char *fullPath, int client){

    //First check if file exists in the directory
    if(!checkIfFileExists(fullPath)){
        printf("File does not exist!\n");
        char *msg = "File does not exist on the server.";
        write(client, msg, strlen(msg) + 1);
        return;
    }else{
        printf("File Exists\n");
        if (remove(fullPath) == -1) { 
            printf("Could not remove File\n");
            char *msg = "Could not remove file from server.";
            write(client, msg, strlen(msg) + 1);
            return;
        }else{
            printf("File Removal Success.");
            char *msg = "File removed succesfully from server.";
            write(client, msg, strlen(msg) + 1);
        }
    }
}

//Shane Change
void removeHandler(char **commandArgv, int commandArgc, int client){
    
    const char *fullPath = constructFullPath(baseDir, commandArgv[1]);
    //printf("FullPath: %s\n", fullPath);

    const char *pathExt = getFileExtension(fullPath);
    //printf("Path Ext: %s\n", pathExt);

    if(pathExt != NULL){
        if (strcmp(pathExt, ".c") == 0) {
            printf("Process for REMOVE .c\n");
            removeCFiles(fullPath, client);
            
        }else if (strcmp(pathExt, ".txt") == 0) {
            printf("Process for REMOVE .txt\n");
        } else if (strcmp(pathExt, ".pdf") == 0) {
            printf("Process for REMOVE .pdf\n");
        }
    }else{
        printf("Invalid file extension provided.\n");
        char *msg = "Invalid file extension provided.";
        write(client, msg, strlen(msg) + 1); // Send Error message to client
        return;
    }
    
}

//Shane Change
int containsCFiles(const char *filePath, const struct stat *FileInfo, int flag, struct FTW *ftwInfo){
    if(flag == FTW_F){ // Checks if is a File
       cFilesExist = true;
       return 1;
    }else{
        return 0;
    }
}

//Shane Change
void tarCFiles(int client){
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current Working Directory %s\n", cwd);  
    }

    //Check if C Files exist on the smain server
    const char *path = cwd;
    //const char *path = "~smain/";
    if(nftw(path, containsCFiles, 50, FTW_PHYS ) == -1){ //If error occurs
        printf("Error occurred while visiting the directory '%s'\n", path);
        return; 
    }

    if(!cFilesExist){
        printf("C files do not exist!\n");
        char *msg = "C files not exist on the server.";
        write(client, msg, strlen(msg) + 1);
        return;
    }

    // Create a tarball name using the Unix timestamp
    const char *tarball_dir = cwd;
    
    if (mkdir(tarball_dir, 0777) == -1) { 
        if (errno != EEXIST) {  // Ignore the error if the directory already exists
            perror("Error creating tarball directory");
            return;
        }
    }

    char tarball_name[1024];
    time_t now = time(NULL);
    snprintf(tarball_name, sizeof(tarball_name), "%s/cTar-%ld.tar",tarball_dir, now);

    // Construct the full tar command
    char command[1024];
    snprintf(command, sizeof(command), "tar -czf %s %s*.c", tarball_name, path);
    //printf("Tar cmd: %s\n", command);

    if (system(command) == -1) {
        printf("Failed to create tarball\n");
        char *msg = "Failed to create backup.";
        write(client, msg, strlen(msg) + 1);
        return;
    }

    printf("Tarball creation success\n");

    //Send to client
    int tarfd = open(tarball_name, O_RDONLY);
    if (tarfd < 0) {
        perror("Failed to open tarball for sending");
        const char *msg = "Failed to open tarball for sending.";
        write(client, msg, strlen(msg) + 1);
        return;
    }

    //Tar File transfer indicator
    write(client, "dtar", strlen("dtar"));
    write(client, tarball_name, strlen(tarball_name) + 1);

    char buffer[1024];
    ssize_t bytes_read, bytes_sent;
    
    while ((bytes_read = read(tarfd, buffer, 1024)) > 0) {
        bytes_sent = send(client, buffer, bytes_read, 0);
        if (bytes_sent < 0) {
            perror("Failed to send tarball to client");
            close(tarfd);
            return;
        }
    }
    if (bytes_read < 0) {
        perror("Failed to read from tarball");
    } 

    close(tarfd);
    printf("Tarball SENT to client succesfully\n");
}

//Shane Change
void tarHandler(char **commandArgv, int commandArgc, int client){
        if (strcmp(commandArgv[1], ".c") == 0) {
            printf("Process for DTAR .c\n");
            tarCFiles(client);
        }else if (strcmp(commandArgv[1], ".txt") == 0) {
            printf("Process for DTAR .txt\n");
        } else if (strcmp(commandArgv[1], ".pdf") == 0) {
            printf("Process for DTAR .pdf\n");
        }
}

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
        printf("\nError establishing connection to %s server\n", servername);
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
    char sendbytesmessage[100];

    if(recv(server, sendbytesmessage, 8, 0) < 0) {
        printf("\nRecv failed: Unable to send files to server\n");
        return 1;
    }

    char* sendmessage = "sendsize";
    int sendmessagebytes = send(client, sendmessage, strlen(sendmessage), 0);
    if (sendmessagebytes < 0) {
        printf("\nError in send bytes message\n");
        return 1;
    }
    // get size of file
    int recbytes = read(client, filesizebuf, 100);
    if (recbytes < 0) {
        printf("\nError receiving filesize\n");
        return 1;
    }

    filesize = atoi(filesizebuf);

    int n;

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

    char* sendmessage = "sendsize";
    int sendmessagebytes = send(client, sendmessage, strlen(sendmessage), 0);
    if (sendmessagebytes < 0) {
        printf("\nError in send bytes message\n");
        return 1;
    }

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

    //Shane Change //Commented
    int commandArgc  = 0;
    char *commandArgv[200]; 
    parseInput(userinput, commandArgv, &commandArgc);
    for (int i = 0; i < commandArgc; i++) {
        printf("commandArgv[%d] = %s\n", i, commandArgv[i]);
    }

    if(strcmp(commandArgv[0], "dfile") == 0){
        printf("Processing for DFile\n");
        downloadHandler(commandArgv, commandArgc, client);
    }
    else if(strcmp(commandArgv[0], "rmfile") == 0){
        printf("Processing for Remove File\n");
        removeHandler(commandArgv, commandArgc, client);
    }
    else if(strcmp(commandArgv[0], "dtar") == 0){
        printf("Processing for Dtar\n");
        tarHandler(commandArgv, commandArgc, client);
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
