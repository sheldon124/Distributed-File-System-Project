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

//Glbal Variabe to keep track if .txt fles exists in the swrver directiry
bool txtFilesExist = false;

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

//Utilit funcion to get the extwnsion of a file by checkong it backwards
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

//Utility function to eztract a file name from a path
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

//Utility Function to chck if a file existss
bool checkIfFileExists(const char *filepath){
    struct stat fileInfo; // Stat func gets directory info

    // S_ISREG macro check the file mode to determinee if it's a file
    return (stat(filepath, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) ? true : false;
}

//Utiity function to constuct the ful absolute pth
char *constructFullPath(const char *path){
    
    //Get the current pwd
    char pwd[1024];
    if (getcwd(pwd, sizeof(pwd)) != NULL) {
        
        //Constructing the full absolute path
        static char fullPath[200];
        strcpy(fullPath, pwd);
        strcat(fullPath, path + 6); //addss after ~smain
        return fullPath;
    }else{
        printf("Current Working Directory can not be determined.\n");  
    }
}

//Function to cgeck if a file is present as only .txt files wil exist on the msin server
int containsTXTFiles(const char *filePath, const struct stat *FileInfo, int flag, struct FTW *ftwInfo){
    
    if(flag == FTW_F){ // Checks if is a File
        //printf("File Path Found: %s\n", filePath);

        //Extrct extension frm psth and deteminw if is a .txt file
        const char *pathExt = getFileExtension(filePath);
        if(pathExt != NULL && strcmp(pathExt, ".txt") == 0){
            txtFilesExist = true;
            return 1;
        }else{
            return 0;
        }
    }else{
        return 0;
    }
}

//Function that reads a files and transfer it to the main server
void downloadHandler(const char *path, int client){
    //Constructing the full path
    const char *fullPath = constructFullPath(path); 
    //printf("Full Path--->%s\n", fullPath);

    //First check if file eists in the directory
    if(!checkIfFileExists(fullPath)){ 
        printf("File does not exist!\n");
        char *errmsg = "File does not exist.";
        write(client, errmsg, strlen(errmsg) + 1);
        return;
    }else{
        //printf("Filex Exists\n");
        char fileBuffer[1024];
        long int bytesRead;
        long int bytesSent;

        //Reading File
        // Opens File Descritor in Read Only permssion from the source file
        int fdSrc = open(fullPath, O_RDONLY); 
        if (fdSrc == -1) { // If can not open file send error message to client
            printf("Error whille opening the file.\n");
            char *errmsg = "File not found on server.";
            write(client, errmsg, strlen(errmsg) + 1); 
            close(fdSrc);
            return;
        }

        //Sening dfile to client, so clint can move to funvtion for proceing the chunkds it receives.
        write(client, "dfile", strlen("dfile") + 1);

        //Reading file in chunks and sending to client
        int chunksSent = 0;
        while( ( bytesRead = read(fdSrc, fileBuffer, 1024) ) > 0 ){ //Read up to 1024 bytes in chunks from the server
            bytesSent = send(client, fileBuffer, bytesRead, 0); //Sending
            if (bytesSent < 0) {  //Iff err occurs send
                printf("Error in sending file\n");
                close(fdSrc);
                return;    
            }else{
                //Checking file size based on chinks
                chunksSent = chunksSent + 1;
                if(chunksSent > 16000){ //16mb
                    //printf("File size exceeds 16mb\n");
                }
            }

            // Clear buffer
            memset(fileBuffer, 0, sizeof(fileBuffer));

        }

        //Error check
        (bytesRead < 0) ? printf("Error occured while receiving file from server\n") : printf("File transfered successfully.\n");
        close(fdSrc);
    }
}

//Function that tars txt files and transfers it to the main server
void tarHandler(int client){

    //Get current working directory of server
    char pwd[1024];
    if (getcwd(pwd, sizeof(pwd)) == NULL) {
        //printf("Current Working Directory %s\n", pwd);  
        char *errmsg = "Server directory error.";
        write(client, errmsg, strlen(errmsg) + 1); 
    }

     //Check if .txt Files exist on the smain server bt traversing using the nftw system call
    const char *path = pwd;
    //const char *path = "~smain/";
    if(nftw(path, containsTXTFiles, 50, FTW_PHYS ) == -1){ //If error occurs whilw traversng
        printf("Error occurred while visiting the directory '%s'\n", path);
        char *errmsg = "Server directory error.";
        write(client, errmsg, strlen(errmsg) + 1); 
        return; 
    }

    //Check if .txt files exist in the directory, if not then send messfe to client
    if(!txtFilesExist){
        printf(".txt files do not exist on the server\n");
        char *errmsg = ".txt files not exist on the server.";
        write(client, errmsg, strlen(errmsg) + 1);
        return;
    }else{
        //Creating tar file name, adding a timestamo to it
        char tarFileName[1024];
        time_t currentTime = time(NULL);
        snprintf(tarFileName, sizeof(tarFileName), "%s/txtTar-%ld.tar",path, time(NULL));

        char tarExe[MAXSIZE];
        if(tarFileName && path){
            snprintf(tarExe, sizeof(tarExe), "tar -czf %s -C %s $(find . -name '*.txt')", tarFileName, path);
        }

        //Executing the tar command
        int result = system(tarExe);
        if (result < 0) { //Error check
            printf("Failed to create tar file\n");
            char *errmsg = "Failed to create tar file.";
            write(client, errmsg, strlen(errmsg) + 1);
            return;
        }else{
            printf("Tar file creation success\n");
        }

        //Send to main
        //Open tar file, witg read permissions
        int tarfd = open(tarFileName, O_RDONLY);
        if (tarfd < 0) { //Erro check and send message t client
            perror("Failed to open tar file for sending");
            const char *errmsg = "Failed to access tar file.";
            write(client, errmsg, strlen(errmsg) + 1);
            return;
        }

        //File transfer indicator so client know tar file download is about to come in
        write(client, tarFileName, strlen(tarFileName) + 1);

        char fileBuffer[1024];
        long int bytesRead;
        long int bytesSent;

        //Reading file in chunks and sending to client
        int chunksSent = 0;
        while( ( bytesRead = read(tarfd, fileBuffer, 1024) ) > 0 ){ //Read up to 1024 bytes in chunks from the server
            bytesSent = send(client, fileBuffer, bytesRead, 0); //Sending
            if (bytesSent < 0) {  //Iff err occurs during send
                printf("Error in sending tar file\n");
                close(tarfd);
                return;    
            }else{
                //Checking file size based on chinks
                chunksSent = chunksSent + 1;
                if(chunksSent > 16000){ //16mb
                    //printf("File size exceeds 16mb\n");
                }
            }

            // Clear buffer
            memset(fileBuffer, 0, sizeof(fileBuffer));
        }

        //Error check
        (bytesRead < 0) ? printf("Error occured while receiving file from server\n") : printf("File transfered successfully.\n");
        close(tarfd);
    }
}

//Function to determine which server will process the removal
void removeHandler(const char *path, int client){

    //Constructing full path
    const char *fullPath = constructFullPath(path);

    //First check if file exists in the directory
    if(!checkIfFileExists(fullPath)){
        printf("File does not exist!\n");
        char *msg = "File does not exist on the server.";
        write(client, msg, strlen(msg) + 1);
        return;
    }else{
        //Remove the file using the remove
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

// Function to display list of files on server
int listfiles(char* userpath, int client) {
    char path[MAXSIZE];
    userpath[0] != '.'? strcpy(path, userpath): strcpy(path, "./");
    struct stat st;
    if(stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) { // check if directory exists
        printf("\nDirectory path does not exist in smain");
    }
    else {
        DIR* directory;
        if (!(directory = opendir(path))) {
            perror("Error opening directory");
            return 1;
        }
        for(struct dirent *direntry = readdir(directory); direntry != NULL; direntry = readdir(directory)) { // loop through files/directories in the path

            if (strcmp(direntry->d_name, ".") == 0 || strcmp(direntry->d_name, "..") == 0) {
            continue;
        }

            char new_path[1024];
            snprintf(new_path, sizeof(new_path), "%s/%s", path, direntry->d_name);

        if (direntry->d_type == DT_REG) { // check if entry is of type file
            printf("%s\n", direntry->d_name);
            int n = send(client, direntry->d_name, strlen(direntry->d_name), 0); // send file name to main
            if(n < 0) {
                printf("Display to client failed\n");
                return 1;
            }
            sleep(0.5);
        }
        }
        closedir(directory);
    }
    char complete[50] = "complete";
    int n = send(client, complete, strlen(complete), 0);
    if (n < 0)
    {
        printf("Display to client failed\n");
        return 1;
    }
    sleep(0.5);

    return 0;
}

// Function to upload files to server
int ufilecommand(char *cmd, char *filename, char *dest, int client) {
    int pathprovided = 1;
    char destpath[MAXSIZE];
    struct stat st;

    if (strcmp(dest, "/") == 0) {
        pathprovided = 0;
        strcpy(destpath, "");
    }
    else
        strcpy(destpath, dest);

    // create directory path if it does not exist
    if (pathprovided && (stat(destpath, &st) != 0 || !S_ISDIR(st.st_mode))) {
        // Directory does not exist, attempt to create it
        char newpath[MAXSIZE] = "";
        int count = 0;
        char temppath[MAXSIZE];
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
    char filesizebuf[MAXSIZE];

    char* sendmessage = "sendsize";
    int sendmessagebytes = send(client, sendmessage, strlen(sendmessage), 0); // ask to get size from client
    if (sendmessagebytes < 0) {
        printf("\nError in send bytes message\n");
        return 1;
    }
    // get size of file
    int recbytes = read(client, filesizebuf, MAXSIZE);
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

    int fd = open(destpath, O_CREAT | O_RDWR | O_APPEND, 0777); // create file if does not exist
    if (fd < -1)
    {
        printf("\nError opening file\n");
        return 1;
    }

    char *message = "received";
    send(client, message, strlen(message), 0);

    int totalbytesread = 0;
    char filebuf[MAXSIZE];

    // Get bytes from main
    while (totalbytesread < filesize)
    {
        int bytesread = recv(client, filebuf, MAXSIZE, 0);
        totalbytesread += bytesread;
        int writebytes = write(fd, filebuf, bytesread);
        if (writebytes < 0)
        {
            printf("\nError writing file\n");
            return 1;
        }
    }

    printf("\nCompleted Upload Functionality\n");

    return 0;
}

// Function to process client request based on input command
int prcclient(char* inputcommand, int client) {
    char cmd[MAXSIZE];
    char filename[MAXSIZE];
    char dest[MAXSIZE];
    sscanf(inputcommand, "%s %s %s", cmd, filename, dest); // get command from main server
    if(strcmp(cmd, "ufile") == 0) { // process ufile command
        return ufilecommand(cmd, filename, dest, client);
    }
    else if(strcmp(cmd, "display") == 0) { // process display command
        return listfiles(filename, client);
    }


    //Splitting command into individual commands for executionn
    int commandArgc  = 0;
    char *commandArgv[200]; 
    commandSplitter(inputcommand, commandArgv, &commandArgc);

    //Command checks to perform respected operations
    if(strcmp(commandArgv[0], "dfile") == 0){ //If command starts with dfile, user wants to download a file
        printf("Processing for dfile in TXT server\n");
        downloadHandler(commandArgv[1], client);
    }
    else if(strcmp(commandArgv[0], "dtar") == 0){ //If command starts with dfile, user wants to download a file
        printf("Processing for dfile in TXT server\n");
        tarHandler(client);
    }else if(strcmp(commandArgv[0], "rmfile") == 0){ //If command starts with dfile, user wants to download a file
        printf("Processing for rmfile in TXT server\n");
        removeHandler(commandArgv[1], client);
    }

    return 0;
}
  
int main(int argc, char *argv[]) {
int sd, client, portNumber;
struct sockaddr_in servAdd;

// Check if the correct arguments number
if(argc != 2){
fprintf(stderr,"Call model: %s <Port#>\n",argv[0]);
exit(0);
}

// Create socket
if((sd = socket(AF_INET, SOCK_STREAM, 0))<0){
fprintf(stderr, "Could not create socket\n");
exit(1);
}

// Initialize the server address structure
servAdd.sin_family = AF_INET;
servAdd.sin_addr.s_addr = htonl(INADDR_ANY);
sscanf(argv[1], "%d", &portNumber);
servAdd.sin_port = htons((uint16_t)portNumber);

// Bind socket to server address
bind(sd, (struct sockaddr *) &servAdd,sizeof(servAdd));

// Listen for incoming connections
if (listen(sd, 5) < 0) {
    perror("listen failed");
    exit(1);
}

while(1) {
    // Accept a connection
    client=accept(sd,(struct sockaddr*)NULL,NULL);//accept()
    if(client < 0) {
        perror("Error accepting connection");
        continue;
    }

    // Create a new process where client is serviced
    int pid = fork();

    // Child process handles the client
    if (pid == 0) {
    char buff1[MAXSIZE];
    int bytes_read = read(client, buff1, MAXSIZE);
    if (bytes_read < 0) {
        printf("Server: read() failure\n");
        exit(3);
    }
    printf("%s\n", buff1);
    prcclient(buff1, client); // Process client's command
    close(client);
    printf("\n");
    exit(0);
} else if (pid > 0) { // Parent continues to accept new connection
    close(client);
}
}

}
