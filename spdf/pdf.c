#define _GNU_SOURCE  // For strsep
#define _XOPEN_SOURCE 500  // To use nftw
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

bool pdfFilesExist = false; //Global Var

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
//Utility Function to chck if a file existss
bool checkIfFileExists1(const char *filepath){
    struct stat fileInfo; // Stat func gets directory info

    // S_ISREG macro check the file mode to determinee if it's a file
    return (stat(filepath, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) ? true : false;
}

//Shane WIP
//Function to check if a file is present as only .pdf files will exist on the main server
int containsPDFFiles(const char *filePath, const struct stat *FileInfo, int flag, struct FTW *ftwInfo){
    
    if(flag == FTW_F){ // Checks if is a File
        //printf("File Path Found: %s\n", filePath);
        const char *pathExt = getFileExtension(filePath);
        if(pathExt != NULL && strcmp(pathExt, ".pdf") == 0){
            pdfFilesExist = true;
            return 1;
        }else{
            return 0;
        }
    }else{
        return 0;
    }
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
//Function that reads a files and transfer it to the main server
void downloadHandler(const char *path, int client){
    //Constructing the full path
    const char *fullPath = constructFullPath(path); //Will Fail if has folder structure
    printf("Absoulute Path of file--->%s\n", fullPath);

    if(!checkIfFileExists1(fullPath)){ 
        printf("File does not exist!\n");
        char *errmsg = "File does not exist.";
        write(client, errmsg, strlen(errmsg) + 1);
        return;
    }else{
        //printf("Filex Exists\n");
        char fileBuffer[1024];
        long int bytesRead;
        long int bytesSent;

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
//Function that tars pdf files and transfers it to the main server
void tarHandler(int client){

    //Get current working directory of server
    char pwd[1024];
    if (getcwd(pwd, sizeof(pwd)) == NULL) {
        //printf("Current Working Directory %s\n", pwd);  
        char *errmsg = "Server directory error.";
        write(client, errmsg, strlen(errmsg) + 1); 
    }

    //Check if C Files exist on the smain server
    const char *path = pwd;
    //const char *path = "~smain/";
    if(nftw(path, containsPDFFiles, 50, FTW_PHYS ) == -1){ //If error occurs
        printf("Error occurred while visiting the directory '%s'\n", path);
        char *errmsg = "Server directory error.";
        write(client, errmsg, strlen(errmsg) + 1); 
        return; 
    }

    if(!pdfFilesExist){
        printf(".pdf files do not exist on the server\n");
        char *errmsg = ".pdf files not exist on the server.";
        write(client, errmsg, strlen(errmsg) + 1);
        return;
    }else{

        char tarFileName[1024];
        time_t now = time(NULL);
        snprintf(tarFileName, sizeof(tarFileName), "%s/pdfTar-%ld.tar",path, now);

        char tarExe[MAXSIZE];
        snprintf(tarExe, sizeof(tarExe), "tar -czf %s -C %s $(find . -name '*.pdf')", tarFileName, path);

        int result = system(tarExe);
        if (result < 0) {
            printf("Failed to create tarball\n");
            char *errmsg = "Failed to create backup.";
            write(client, errmsg, strlen(errmsg) + 1);
            return;
        }else{
            printf("Tar file creation success\n");
        }

        int tarfd = open(tarFileName, O_RDONLY);
        if (tarfd < 0) {
            perror("Failed to open tar file for sending");
            const char *errmsg = "Failed to access tar file.";
            write(client, errmsg, strlen(errmsg) + 1);
            return;
        }

        //File transfer indicator
        char fileBuffer[1024];
        long int bytesRead;
        long int bytesSent;
        write(client, tarFileName, strlen(tarFileName));

        //Reading file in chunks and sending to client
        while( ( bytesRead = read(tarfd, fileBuffer, 1024) ) > 0 ){ //Read up to 1024 bytes and will continue until all bytes read
            bytesSent = send(client, fileBuffer, bytesRead, 0); //0 is for flag
            if (bytesSent < 0) {  //0 bytes must not be sent, must always be more then 0
                printf("Error in sending tar file\n");
                close(tarfd);
                return;    
            } 
        }

        //Error check
        printf(bytesRead < 0 ? "Error occured while receiving tar file from server" : "Tar file succesfullly sent from server.\n");

        close(tarfd);

    }
}

//Sheldon
bool checkIfFileExists(const char *filepath){
    struct stat fileInfo;
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        return false;
    }

    close(fd);
    return true;
}

//Sheldon
int listfiles(char* userpath, int client) {
    char path[100];
    userpath[0] != '.'? strcpy(path, userpath): strcpy(path, "./");
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
    char complete[50] = "complete";
    // complete[strlen(complete)] = '\0';
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

    char* sendmessage = "sendsize";
    int sendmessagebytes = send(client, sendmessage, strlen(sendmessage), 0);
    if (sendmessagebytes < 0) {
        printf("\nError in send bytes message\n");
        return 1;
    }
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

//Sheldon
int removefile(char* filepath) {
    if(checkIfFileExists(filepath)) {
        remove(filepath);
        return 0;
    }
    printf("\nFile does not exist\n");
    return -1;
}

int prcclient(char* inputcommand, int client) {
    char cmd[100];
    char filename[100];
    char dest[100];
    sscanf(inputcommand, "%s %s %s", cmd, filename, dest);
    if(strcmp(cmd, "ufile") == 0) {
        return ufilecommand(cmd, filename, dest, client);
    }
    else if(strcmp(cmd, "display") == 0) {
        return listfiles(filename, client);
    }
    else if(strcmp(cmd, "rmfile") == 0) {
        if(removefile(filename) < 0) {
            char* status = "0";
            send(client, status, 1, 0);
        }
        else {
            char* status = "1";
            send(client, status, 1, 0);
        }
    }

    //Shane Change 
    int commandArgc  = 0;
    char *commandArgv[200]; 
    commandSplitter(inputcommand, commandArgv, &commandArgc);

    if(strcmp(commandArgv[0], "dfile") == 0){ //If command starts with dfile, user wants to download a file
        printf("Processing for dfile in PDF server\n");
       downloadHandler(commandArgv[1], client);
    }
    else if(strcmp(commandArgv[0], "dtar") == 0){ //If command starts with dfile, user wants to download a file
        printf("Processing for dfile in pdf server\n");
        tarHandler(client);
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
