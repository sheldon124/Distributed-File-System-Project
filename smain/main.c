#define _GNU_SOURCE  // for strsep
#define _XOPEN_SOURCE 500  // for nftw
#include <stdio.h> 
#include <stdlib.h> 
#include <ftw.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <unistd.h>  
#include <string.h> 
#include <stdint.h> 
#include <limits.h> 
#include <fcntl.h>
#include <errno.h> 
#include <stdbool.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <regex.h>
#include <time.h>
#include <dirent.h>
#define MAXSIZE 1024

int connecttoserver(char* command, char* servername);

//Shane WIP
bool cFilesExist = false;
char *FilesFound[MAXSIZE];
int totalFilesFound = 0;

//Shane WIP
// Utility function to break down commands into indivudal commands to prepare for execution
void commandSplitter(char *input, char **commandArgv, int *commandArgc) {
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

//Shane WIP
//Utility Function to chck if a file existss
bool checkIfFileExists(const char *filepath){
    struct stat fileInfo; // Stat func gets directory info

    // S_ISREG macro check the file mode to determinee if it's a file
    return (stat(filepath, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) ? true : false;
}

//Shane WIP
//Utility function to extract a file name
const char* extractFileName(const char* path){
    const char *fileName = NULL;
    for(int i = strlen(path); i>=0; i--){
        if(path[i] == '/'){ //abc.pdf => .pdf
            fileName = &path[i]; //Store in the pathExt variable
            break; // Exit the loop
        }
    }
    return fileName;
}

//Shane WIP
//Utility function to construct the full absolute path
char *constructFullPath(const char *path){
    
    //Get the current pwd
    char pwd[1024];
    if (getcwd(pwd, sizeof(pwd)) != NULL) {
        //printf("Current Working Directory %s\n", pwd);  
        //Constructing the full absolute path
        static char fullPath[200];
        strcpy(fullPath, pwd);
        strcat(fullPath, path + 6); //Append after ~smain
        return fullPath;
    }else{
        printf("Current Working Directory can not be determined.\n");  
    }
}


//Shane WIP
//Utility function to get the extension of a file by checking it backwards
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

//Shane WIP
//Function that reads a files and transfer it to the client
void downloadCFiles(const char *fullPath, int client){

    //First check if file exists in the directory
    if(!checkIfFileExists(fullPath)){ 
        printf("File does not exist!\n");
        char *errmsg = "File does not exist on the server.";
        write(client, errmsg, strlen(errmsg) + 1);
        return;
    }else{
        char fileBuffer[1024];
        long int bytesRead;
        long int bytesSent;

        //Reading File
        // Opens File Descriptor in Read Only permission from the source file
        int fdSrc = open(fullPath, O_RDONLY); 
        if (fdSrc == -1) { // If can not open file send error message to client
            printf("Error whille opening the file.\n");
            char *errmsg = "File not found on server.";
            write(client, errmsg, strlen(errmsg) + 1); 
            close(fdSrc);
            return;
        }

        //File transfer indicator
        write(client, "dfile", strlen("dfile"));

        //Reading file in chunks and sending to client
        while( ( bytesRead = read(fdSrc, fileBuffer, 1024) ) > 0 ){ //Read up to 1024 bytes and will continue until all bytes read
            bytesSent = send(client, fileBuffer, bytesRead, 0); //0 is for flag
            if (bytesSent < 0) {  //0 bytes must not be sent, must always be more then 0
                printf("Error in sending file\n");
                close(fdSrc);
                return;    
            } 
        }

        //Error check
        printf(bytesRead < 0 ? "Error occured while receiving file from server" : "File succesfullly sent from server.\n");

        close(fdSrc);
    }
}

//Shane WIP
//Function to download files from text and pdf server
//Then transfer to client
void downloadFromServers(char* command, char* servername, int client) {
    
    int server = connecttoserver(command, servername);
    if (server < 0) {
        printf("\nError establishing connection to %s server\n", servername);
        return;
        //return -1;
    }

    char fileBuffer[1024];
    char serverRes[MAXSIZE];
    int bytesRead = read(server, serverRes, MAXSIZE - 1);
    
    if (bytesRead < 0) {
        printf("Client: read() failure\n");
        exit(3);
    }
    serverRes[bytesRead] = '\0';
    printf("External Server: %s\n", serverRes);

    if (strcmp(serverRes, "dfile") == 0){
        write(client, "dfile", strlen("dfile"));

        while( ( bytesRead = read(server, fileBuffer, sizeof(fileBuffer) ) ) > 0 ){ //Read up to 1024 bytes and will continue until all bytes read
            long int bytesSent = send(client, fileBuffer, bytesRead, 0); //0 is for flag
            if (bytesSent < 0) {  //0 bytes must not be sent, must always be more then 0
                printf("Error in sending file\n");
                close(server);
                return;    
            } 
        }

        //Error check
        printf(bytesRead < 0 ? "Error occured while receiving file from server" : "File succesfullly sent to Client.\n");
    }
    else if ( strlen(serverRes) >= 4 && strcmp(serverRes + strlen(serverRes) - 4, ".tar") == 0 ) { //if dtar is received, means it has to expect a download tar file        
        write(client, serverRes, strlen(serverRes));
        
        while( ( bytesRead = read(server, fileBuffer, sizeof(fileBuffer) ) ) > 0 ){ //Read up to 1024 bytes and will continue until all bytes read
            long int bytesSent = send(client, fileBuffer, bytesRead, 0); //0 is for flag
            if (bytesSent < 0) {  //0 bytes must not be sent, must always be more then 0
                printf("Error in sending file\n");
                close(server);
                return;    
            } 
        }

        //Error check
        printf(bytesRead < 0 ? "Error occured while receiving file from server" : "File succesfullly sent to Client.\n");
    }
    else{
        write(client, serverRes, strlen(serverRes) + 1);
    }
    
    close(server);
}


//Shane WIP
// Function to initiate downloading a file from the respected server and transfered to the client
void downloadHandler(char **commandArgv, int commandArgc, int client){
    //Converting argv into a single command with spaces after each command
    char buffer[MAXSIZE];
    buffer[0] = '\0';
    for (int i = 0; i < commandArgc; i++) {
        strcat(buffer, commandArgv[i]);  
        if (i < commandArgc - 1) { //Add space after every command
            strcat(buffer, " "); 
        }
    }

    //printf("Buffer: %s\n ", buffer);
    
    //Constructing the full path
    const char *fullPath = constructFullPath(commandArgv[1]);

    //Extracting file extension
    const char *pathExt = getFileExtension(fullPath);

    //File extension check to see which server to send to
    if (strcmp(pathExt, ".c") == 0) {
        //printf("Process for .c\n");
        downloadCFiles(fullPath, client);
    } else if (strcmp(pathExt, ".txt") == 0) {
        printf("Process for .txt\n");
        downloadFromServers(buffer, "txt" , client);
    } else if (strcmp(pathExt, ".pdf") == 0) {
        downloadFromServers(buffer, "pdf" , client);
    }
    
}

//Shane WIP
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

//Shane WIP
int removeHandler(char **commandArgv, int commandArgc, int client){

    //Converting argv into a single command with spaces after each command
    char buffer[MAXSIZE];
    buffer[0] = '\0';
    for (int i = 0; i < commandArgc; i++) {
        strcat(buffer, commandArgv[i]);  
        if (i < commandArgc - 1) { //Add space after every command
            strcat(buffer, " "); 
        }
    }
    
    const char *fullPath = constructFullPath(commandArgv[1]);
    //printf("FullPath: %s\n", fullPath);

    const char *pathExt = getFileExtension(fullPath);
    //printf("Path Ext: %s\n", pathExt);

    if(pathExt != NULL){
        if (strcmp(pathExt, ".c") == 0) {
            printf("Process for REMOVE .c\n");
            removeCFiles(fullPath, client);
            
        }else if (strcmp(pathExt, ".txt") == 0) {
            printf("Process for REMOVE .txt\n");
            downloadFromServers(buffer, "txt" , client);
        } else if (strcmp(pathExt, ".pdf") == 0) {
            printf("Process for REMOVE .pdf\n");
            printf("Process for REMOVE .pdf\n");
            char *mainfolder = "~smain";

            char command[100];

            char path[100];
            strcpy(path, commandArgv[1] + strlen(mainfolder) + 1);
            strcpy(command, commandArgv[0]);
            strcat(command, " ");
            strcat(command, path);
            strcat(command, " ");

            int server = connecttoserver(command, "pdf");
            if (server < 0) {
                printf("\nError establishing connection to pdf server\n");
                return -1;
            }
            char status[1];
            int recvstatusbytes = recv(server, status, 1, 0);
            if(recvstatusbytes < 0) {
                printf("\nError deleting file\n");
                close(server);
                return -1;
            }
            printf("%s\n", status);
            if(status[0] == '0') {
                printf("\nFile does not exist\n");
            }
            else {
                printf("\nFile removed succesfully from server\n");
            }
            close(server);
        }
    }else{
        printf("Invalid file extension provided.\n");
        char *msg = "Invalid file extension provided.";
        write(client, msg, strlen(msg) + 1); // Send Error message to client
        return -1;
    }
    return 0;
}

//Shane WIP
//Function to check if a file is present as only .c files will exist on the main server
int containsCFiles(const char *filePath, const struct stat *FileInfo, int flag, struct FTW *ftwInfo){
    
    if(flag == FTW_F){ // Checks if is a File
        //printf("File Path Found: %s\n", filePath);
        const char *pathExt = getFileExtension(filePath);
        if(pathExt != NULL && strcmp(pathExt, ".c") == 0){
            cFilesExist = true;
            return 1;
            // if(totalFilesFound < MAXSIZE){
            //     FilesFound[totalFilesFound] = strdup(filePath); //Add files  to the array
            //     totalFilesFound++; //Increment, so other file paths can be added
            // }else{ //If array size falls shot
            //     printf("Files Found array is full.\n");
            //     return 1;
            // }
        }else{
            return 0;
        }
    }else{
        return 0;
    }
}


//Shane WIP
//Functio to create tar files and send to client
void tarCFiles(int client){

    //Get current working directory of server
    char pwd[1024];
    if (getcwd(pwd, sizeof(pwd)) == NULL) {
        //printf("Current Working Directory %s\n", pwd);  
        char *errmsg = "Server directory error.";
        write(client, errmsg, strlen(errmsg) + 1); 
    }

    //Check if C Files exist on the smain server
    const char *path = pwd;
    if(nftw(path, containsCFiles, 50, FTW_PHYS ) == -1){ //If error occurs
        printf("Error occurred while visiting the directory '%s'\n", path);
        char *errmsg = "Server directory error.";
        write(client, errmsg, strlen(errmsg) + 1); 
        return; 
    }

    if(!cFilesExist){
        printf("C files do not exist!\n");
        char *errmsg = "C files not exist on the server.";
        write(client, errmsg, strlen(errmsg) + 1);
        return;
    }

    //Creatin tar file name
    char tarFileName[1024];
    time_t now = time(NULL);
    snprintf(tarFileName, sizeof(tarFileName), "%s/cTar-%ld.tar",path, now);

    //Preparing Tar executable command
    char tarExe[MAXSIZE];
    //snprintf(tarExe, sizeof(tarExe), "tar -czf %s", tarFileName);
    // for (int i = 0; i < totalFilesFound; i++) {
    //     strcat(tarExe, " ");
    //     strcat(tarExe, FilesFound[i]);
    // }
    snprintf(tarExe, sizeof(tarExe), "tar -czf %s -C %s $(find . -name '*.c')", tarFileName, path);

    //Executing the tar command
    int result = system(tarExe);
    if (result < 0) {
        printf("Failed to create tarball\n");
        char *errmsg = "Failed to create backup.";
        write(client, errmsg, strlen(errmsg) + 1);
        return;
    }else{
        printf("Tar file creation success\n");
    }

    //Send to client
    int tarfd = open(tarFileName, O_RDONLY);
    if (tarfd < 0) {
        perror("Failed to open tar file for sending");
        const char *errmsg = "Failed to access tar file.";
        write(client, errmsg, strlen(errmsg) + 1);
        return;
    }

    //File transfer indicator
    write(client, tarFileName, strlen(tarFileName));

    char tarBuffer[MAXSIZE];
    long int bytesRead;
    long int bytesSent;

    //Reading file in chunks and sending to client
    while( ( bytesRead = read(tarfd, tarBuffer, 1024) ) > 0 ){ //Read up to 1024 bytes and will continue until all bytes read
        bytesSent = send(client, tarBuffer, bytesRead, 0); //0 is for flag
        if (bytesSent < 0) {  //0 bytes must not be sent, must always be more then 0
            printf("Error in sending file\n");
            close(tarfd);
            return;    
        } 
    }

    //Error check
    printf(bytesRead < 0 ? "Error occured while receiving file from server" : "File succesfullly sent from server.\n");

    close(tarfd);
}

//Shane WIP
//Function to handle tar distribution via extensions
void tarHandler(char **commandArgv, int commandArgc, int client){

    //Converting argv into a single command with spaces after each command
    char buffer[MAXSIZE];
    buffer[0] = '\0';
    for (int i = 0; i < commandArgc; i++) {
        strcat(buffer, commandArgv[i]);  
        if (i < commandArgc - 1) { //Add space after every command
            strcat(buffer, " "); 
        }
    }


    //Check the file extension to know on which server processing must be done
    if (strcmp(commandArgv[1], ".c") == 0) {
        printf("Process for DTAR .c\n");
        tarCFiles(client);
    }else if (strcmp(commandArgv[1], ".txt") == 0) {
        printf("Process for DTAR .txt\n");
        downloadFromServers(buffer, "txt" , client);
    } else if (strcmp(commandArgv[1], ".pdf") == 0) {
        printf("Process for DTAR .pdf\n");
        downloadFromServers(buffer, "pdf" , client);
    }
}

//Sheldon
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

//Sheldon
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

//Sheldon
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

//Sheldon
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

//Sheldon
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

//Sheldon
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

int prcclient(char* userinput, int client) {
    char cmd[100];
    char filename[100];
    char dest[100];
    char* mainfolder = "~smain";
    char destpath[100];
    sscanf(userinput, "%s %s %s", cmd, filename, dest);
    printf("%s, %s, %s\n", cmd, filename, dest);

    // ufile command
    if (strcmp(cmd, "ufile") == 0) {
        int success = ufilecommand(cmd, filename, dest, client);
        if (success == 0) {
            char * message = "Upload Successful";
            send(client, message, strlen(message), 0);
        } else {
            char * message = "Upload Failed";
            send(client, message, strlen(message), 0);
        }
    } else if (strcmp(cmd, "display") == 0) {
        int success = listfiles(filename, client);
        // if (success == 0) {
        //     char * message = "Successful";
        //     send(client, message, strlen(message), 0);
        // } else {
        //     char * message = "Failed";
        //     send(client, message, strlen(message), 0);
        // }
    }

    //Shane WIP 
    int commandArgc  = 0;
    char *commandArgv[200]; 
    commandSplitter(userinput, commandArgv, &commandArgc);

    //Command checks to perform respected operations
    if(strcmp(commandArgv[0], "dfile") == 0){ //If command starts with dfile, user wants to download a file
        printf("Processing for dfile\n");
        downloadHandler(commandArgv, commandArgc, client);
    }
    else if(strcmp(commandArgv[0], "rmfile") == 0){ //If command starts with rmfile, user wants to remove a file
        printf("Processing for rmile\n");
        removeHandler(commandArgv, commandArgc, client);
    }
    else if(strcmp(commandArgv[0], "dtar") == 0){ //If command starts with dtar, user wants to download a tar file
        printf("Processing for dtar\n");
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

    int success = prcclient(buff1, client);


    close(client);
    printf("\n");
    exit(0);
} else if (pid > 0) {
    close(client);
}
}

}
