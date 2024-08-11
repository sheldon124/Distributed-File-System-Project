
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regex.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 3400
#define MAXSIZE 1024

//Shane Change
bool checkIPFormat(const char *ip) {
    regex_t regex;
    const char *pattern = "^([0-9]{1,3}\\.){3}[0-9]{1,3}$";

    if(regcomp(&regex, pattern, REG_EXTENDED) == 0){
        int verificationResult = regexec(&regex, ip, 0, NULL, 0);
        regfree(&regex);
        return (verificationResult == 0) ? true : false; 
    }else{
        return false;
    }
}

//Shane Change
void trimAndRemoveNewLine(char *input){
    if(input[0] != '\0' && input != NULL){ //Check if first character is not empty, n has characters

        //Index variables
        int startIndex = 0;
        int endIndex = strlen(input) - 1;

        //Replace New lin with Null and updting endIndex as can be --> "dter \n"
        if(input[endIndex] == '\n'){
            input[endIndex] = '\0';
            endIndex--;
        }

        // Identifying the spaces from index 0 and updating count
        while (input[startIndex] == ' ') {
            startIndex++;
        }

        // Identifying the spaces from last index nd updating count in revers
        while (input[endIndex] == ' ') {
            endIndex--;
        }
        
        //Shifting the start and end index values to the start
        int j = 0;
        int i = startIndex;
        while(i<= endIndex){
            input[j] = input[i];
            j++;
            i++;
        }
        input[j] = '\0'; //Null terminator mst be added to strings
    }
}  

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
bool checkFileExtension(const char *file){
    if(file == NULL){
        return false;
    }

    const char *pathExt = NULL;
    for(int i = strlen(file); i>=0; i--){

        // Identifying the first dot from reverse
        if(file[i] == '.'){ //abc.pdf => .pdf
            //Store in the pathExt variable
            pathExt = &file[i];

            break; // Exit the loop
        }
    }

    // No dot found or the dot is the first character
    if (pathExt == NULL || pathExt == file) {
        return false; 
    }

    // Compare the extension with the allowed ones
    if (strcmp(pathExt, ".txt") == 0 || strcmp(pathExt, ".pdf") == 0 || strcmp(pathExt, ".c") == 0) {
        return true;
    } else {
        return false;
    }
}

//Shane Change
bool checkTildePath(const char *path){
    if(path == NULL){
        return false;
    }

    const char *prefix = "~smain";
    size_t prefixLen = strlen(prefix);

    //Check if path starts with ~smain
    if (strncmp(path, prefix, prefixLen) != 0) {
        return false;  
    }

    //Allow simple ~smain
    if(path[prefixLen] == '\0' ){
        return true;
    }

    //If there is any other character other then / after ~smain
    if (path[prefixLen] != '/') {
        return false; 
    }

    return true;
}

//Shane Change
bool checkInput(char **commandArgv, int commandArgc){
    if(commandArgc < 1){
        return false;
    }

    // printf("argc = %d\n", argc);
    // for (int i = 0; i < argc; i++) {
    //     printf("commandArgv[%d] = %s\n", i, commandArgv[i]);
    // }

    if(strcmp(commandArgv[0], "ufile") == 0){

        //Check the second arg is a file and has correct file extension
        if(!checkFileExtension(commandArgv[1])){
            printf("Second command must be a file with extension, like sample.c\n");
            printf("Note: Only .c .pdf .txt files allowed\n");
            return false;
        }

        //Check the tilde expansion path
        if(!checkTildePath(commandArgv[2])){
            printf("Second command must be a path, like ~smain/folder1/folder2\n");
            printf("Note: path must begin with ~smain \n");
            return false;
        }
    }
    else if(strcmp(commandArgv[0], "dfile") == 0 ){
        //Check the tilde expansion path
        if(!checkTildePath(commandArgv[1])){
            printf("Second command must be a path, like ~smain/folder1/folder2/sample.c\n");
            printf("Note: path must begin with ~smain \n");
            return false;
        }

        if(!checkFileExtension(commandArgv[1])){
            printf("Second command must be a file with extension, like sample.c\n");
            printf("Note: Only .c .pdf .txt files allowed\n");
            return false;
        }
    }
    else if(strcmp(commandArgv[0], "rmfile") == 0 ){
        //Check the tilde expansion path
        if(!checkTildePath(commandArgv[1])){
            printf("Second command must be a path, like ~smain/folder1/folder2\n");
            printf("Note: path must begin with ~smain \n");
            return false;
        }
    }
    else if(strcmp(commandArgv[0], "display") == 0 ){
        //Check the tilde expansion path
        if(!checkTildePath(commandArgv[1])){
            printf("Second command must be a path, like ~smain/folder1/folder2\n");
            printf("Note: path must begin with ~smain \n");
            return false;
        }
    }
    else if(strcmp(commandArgv[0], "dtar") == 0){
        //printf("ARG COUNT RECEIVED: %d\n", commandArgc);
        if(commandArgc > 2){
            printf("Only 2 arguments are allowed.\n");
            return false;
        }
        if (strcmp(commandArgv[1], ".c") != 0 && strcmp(commandArgv[1], ".pdf") != 0 && strcmp(commandArgv[1], ".c") != 0) {
            printf("Invalid extension '%s'. Please enter '.txt', '.pdf', or '.c'.\n", commandArgv[1]);
            return false;
        }
    }else{
        printf("Invalid command passed.\n");
        return false;
    }

    return true;
}

int displayfiles(int socket) {
    char filename[100];
    printf("\nList of files\n");
    int filespresent = 0;
    int sizereceived;
    while(((sizereceived = recv(socket, filename, 100, 0))) > 0) {
        filename[sizereceived] = '\0';
        if(strcmp(filename, "complete") == 0) {
            break;
        } 
        else {
            printf("%s\n", filename);
            filespresent = 1;
        }
    }

    if(!filespresent) 
    printf("\nNo Files present for the directory\n");
}

//Shane Change
const char* extractFileName(const char* path){
    const char *fileName = NULL;
    for(int i = strlen(path); i>=0; i--){
        if(path[i] == '/'){ //abc.pdf => .pdf
            //Store in the pathExt variable
            fileName = &path[i];

            break; // Exit the loop
        }
    }

    return fileName;
}

//Shane Change
char* createDownloadsPath(const char *filePath) {

    // Create the downloads directory, if not there
    if (mkdir("/home/correas/Project/Distributed-File-System-Project/client/downloads/", 0777) == -1) { 
        // Check if the error occurred due to the directory already existing
        if (errno != EEXIST) {
            perror("Error creating downloads directory");
            return NULL;
        }
    }

    const char *fileName = extractFileName(filePath);

    static char fullDownloadPath[1024];// Buffer
    strcpy(fullDownloadPath, "/home/correas/Project/Distributed-File-System-Project/client/downloads/");
    strcat(fullDownloadPath, fileName);

    return fullDownloadPath;
}

//Shane Change
void downloadingFile(int server, const char *filePath){

    //Create the downloads directory, if not there
    char *fullDownloadPath = createDownloadsPath(filePath);
    if (fullDownloadPath == NULL) {
        printf("Download path could not be created.\n");
        return;
    }
    
    char buffer[1024];
    ssize_t bytes_received;

    umask(0000);
    int file_fd = open(fullDownloadPath, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (file_fd < 0) {
        perror("Error opening file for writing");
        return;
    }

    //
    while ((bytes_received = recv(server, buffer, sizeof(buffer), 0)) > 0) {
        if (write(file_fd, buffer, bytes_received) < 0) {
            perror("Error writing to file");
            close(file_fd);
            return;
        }
    }

    if (bytes_received < 0) {
        perror("Error receiving file");
    } else {
        printf("File received successfully.\n");
    }

    close(file_fd);

}

//Shane Change
char* createTarPath(const char *tarPath) {

    // Create the downloads directory, if not there
    if (mkdir("/home/correas/Project/Distributed-File-System-Project/client/tarFiles/", 0777) == -1) { 
        // Check if the error occurred due to the directory already existing
        if (errno != EEXIST) {
            perror("Error creating tarFiles directory");
            return NULL;
        }
    }

    const char *fileName = extractFileName(tarPath);

    static char fullDownloadPath[1024];// Buffer
    strcpy(fullDownloadPath, "/home/correas/Project/Distributed-File-System-Project/client/tarFiles/");
    strcat(fullDownloadPath, fileName);

    return fullDownloadPath;
}

//Shane Change
void downloadingTarFile(int server){
    char buffer[1024];
    ssize_t bytes_received;

    char tarball_name[1024];
    bytes_received = recv(server, tarball_name, sizeof(tarball_name), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive tarball filename");
        return;
    }
    printf("Receiving file: %s\n", tarball_name);

    char *fullDownloadTarPath = createTarPath(tarball_name);
    if (fullDownloadTarPath == NULL) {
        printf("Download path could not be created.\n");
        return;
    }

    umask(0000);
    int file_fd = open(fullDownloadTarPath, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (file_fd < 0) {
        perror("Error opening file for writing");
        return;
    }

    while ((bytes_received = recv(server, buffer, sizeof(buffer), 0)) > 0) {
        if (write(file_fd, buffer, bytes_received) < 0) {
            perror("Error writing to file");
            close(file_fd);
            return;
        }
    }

    if (bytes_received < 0) {
        perror("Error receiving file");
    } else {
        printf("Tar File received successfully: %s\n", fullDownloadTarPath);
    }

    close(file_fd);

}

//Shane Change
void handleServerResponse(int server, char **commandArgv) {
    char header[1024];
    ssize_t bytes_received;

    bytes_received = recv(server, header, 1024 - 1, 0);
    if (bytes_received < 0) {
        perror("Error receiving header");
        return;
    }

    header[bytes_received] = '\0';

    if (strcmp(header, "dfile") == 0) {
        printf("File transfer initiated by server.\n");
        downloadingFile(server, commandArgv[1]);
    } else if (strcmp(header, "dtar") == 0 || strncmp(header, "dtar", 4) == 0) {
        printf("Dtar transfer initiated by server.\n");
        downloadingTarFile(server);
    }
    else {
        printf("Normal Message from Server: %s\n", header);
    }
}

int uploadfile(int socket, char* filename) {
    int fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("\nfile does not exist\n");
    }

    int filelen = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);

    char leninstr[50];

    sprintf(leninstr, "%d", filelen);


    int n;

    // n= write(socket, leninstr, strlen(leninstr));
    n= send(socket, leninstr, strlen(leninstr), 0);
    
    if(n < 0) {
        printf("\nSend filesize to server failed\n");
        close(fd);
        return 1;
    }

    char confirmation[8];
    int recvconfirm = recv(socket, confirmation, 8, 0);
    if(recvconfirm < 0) {
        printf("\nError uploading file\n");
        close(fd);
        return 1;
    }

    char readbuf[100];

    int readbytes;
    while ((readbytes = read(fd, readbuf, 100)) > 0) {
        n = send(socket, readbuf, readbytes, 0);
        if(n < 0) {
            printf("\nWrite Failed\n");
        }
    }

    printf("\nDone\n");

    close(fd);
    return 0;
}

int main(int argc, char *argv[]){

    int server;
    int portNumber;
    char message[MAXSIZE];
    struct sockaddr_in servAdd;
    char userCommand[MAXSIZE];
    int commandArgc  = 0;
    char *commandArgv[200]; 

    //Check if User provided IP and PORT Number
    if(argc != 3){
        printf("ERROR: IP Address or PORT Number missing \n");
        printf("Please enter command line:%s <IP Address> <Port#>\n",argv[0]);
        exit(0);
    }

    //Checking Ip Address format
    if(!checkIPFormat(argv[1])){
        printf("Incorrect IP Address.\n");
        printf("To know your current IP Address in terminal type $ hostname -i \n");
        exit(1);
    }

    //Create Socket
    if( (server = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
        printf("Failed to create Socket.\n");
        exit(2);
    }

    //Connecting IP and PORT Number on Server Object.
    servAdd.sin_family = AF_INET; //Internet
    sscanf(argv[2], "%d", &portNumber);
    servAdd.sin_port = htons((uint16_t)portNumber);//Port number

    if( inet_pton(AF_INET, argv[1], &servAdd.sin_addr) < 0 ){ //IP Address Connection
        printf("inet_pton() failure.\n");
        exit(3);
    }

    //Connect System Call
    if(connect(server, (struct sockaddr *) &servAdd,sizeof(servAdd))<0){//Connect()
        printf("connect() failure.\n");
        exit(4);
    }

    //Display available commands
    printf("Select any of the commands to run on the smain server: \n");
    printf("1. Upload a file to specific path. $ ufile <filename> <destination_path>\n");
    printf("2. Download a file from Server. $ dfile <filename>\n");
    printf("3. Delete file from server. $ rmfile <filename>\n");
    printf("4. Create and download tar file of .c .txt .pdf file types. $ dtar <file extesion>\n");
    printf("5. Display directory files on the server. $ display <directory_path>\n");
    printf("Please note 'file names' and 'paths' must be a tilde expansion path\n");

    while(1){ //Infinite loop start
        bool validInput = false;

        while(!validInput){
            printf("Enter command: \n");
            fgets(userCommand, sizeof(userCommand), stdin);

            trimAndRemoveNewLine(userCommand); 
            printf("User Command: %s\n", userCommand);

            
            parseInput(userCommand, commandArgv, &commandArgc);

            validInput = checkInput(commandArgv, commandArgc);

            // if(!validInput){
            //     printf("Command is invalid. Please try again.\n");
            // }
        }


        //Converting argv into a single buffer
        char buffer[MAXSIZE];
        buffer[0] = '\0';
        for (int i = 0; i < commandArgc; i++) {
            strcat(buffer, commandArgv[i]);  
            if (i < commandArgc - 1) {
                strcat(buffer, " "); 
            }
        }

        //write(server, userCommand, strlen(userCommand) + 1); // Include null terminator in write
        ssize_t bytes_written = write(server, buffer, strlen(buffer) + 1); // Include null terminator in write
        if (bytes_written < 0) {
            printf("Client: write() failure\n");
            exit(4);
        }

        if(strcmp(commandArgv[0], "ufile") == 0) {
            uploadfile(server, commandArgv[1]);
        }
        else if(strcmp(commandArgv[0], "display") == 0) {
            displayfiles(server);
        }
        else if (strcmp(commandArgv[0], "dfile") == 0 || strcmp(commandArgv[0], "dtar") == 0) {
            handleServerResponse(server, commandArgv);
        }

        //Shane Change
        //Read from pipe and display
        // int bytes_read = read(server, message, MAXSIZE - 1);
        // if (bytes_read < 0) {
        //     printf("Client: read() failure\n");
        //     exit(3);
        // }
        // message[bytes_read] = '\0';
        // printf("Server: %s\n", message);

        
    } //Infinite loop end

    exit(0);
    return 0;
}