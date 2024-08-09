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

#define PORT 3400
#define MAXSIZE 1024

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

void parseInput(char *input, char **argv, int *argc) {
    int index = 0;
    char *currentCmd;
    //Iterates and seperate the commands based on " " identified
    while ((currentCmd = strsep(&input, " ")) != NULL) { 
        if (*currentCmd != '\0') { //Empty check
            argv[index] = currentCmd; //Adds cCarrent cmmand in argv
            //printf("Arg Index---> %s\n", argv[index]);
            index++;
        }
    }

    argv[index] = NULL; // Addin Null terminate the last index is necessaru
    *argc = index; //Assigning the value of index as count ref
}

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

void checkInput(char **argv, int argc){
    if(argc < 1){
        return;
    }

    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    if(strcmp(argv[0], "ufile") == 0){

        //Check the second arg is a file and has correct file extension
        if(!checkFileExtension(argv[1])){
            printf("Second command must be a file with extension, like sample.c\n");
            printf("Note: Only .c .pdf .txt files allowed\n");
            return;
        }

        //Check the tilde expansion path
        if(!checkTildePath(argv[2])){
            printf("Second command must be a path, like ~smain/folder1/folder2\n");
            printf("Note: path must begin with ~smain \n");
            return;
        }
    }
    else if(strcmp(argv[0], "dfile") == 0  || strcmp(argv[0], "rmfile") == 0 || strcmp(argv[0], "display") == 0 ){
        //Check the tilde expansion path
        if(!checkTildePath(argv[1])){
            printf("Second command must be a path, like ~smain/folder1/folder2\n");
            printf("Note: path must begin with ~smain \n");
            return;
        }
    }
    else if(strcmp(argv[0], "dtar") == 0){
        if (strcmp(argv[1], ".c") != 0 && strcmp(argv[1], ".pdf") != 0 && strcmp(argv[1], ".c") != 0) {
            printf("Invalid extension '%s'. Please enter '.txt', '.pdf', or '.c'.\n", argv[1]);
            return;
        }
    }else{
        printf("Invalid command passed.\n");
        return;
    }
}

int main(int argc, char *argv[]){

    int server;
    int portNumber;
    char message[1024];
    struct sockaddr_in servAdd;

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
        exit(0);
    }

    //Create Socket
    if( (server = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
        printf("Failed to create Socket.\n");
        exit(1);
    }

    //Connecting IP and PORT Number on Server Object.
    servAdd.sin_family = AF_INET; //Internet
    sscanf(argv[2], "%d", &portNumber);
    servAdd.sin_port = htons((uint16_t)portNumber);//Port number

    if( inet_pton(AF_INET, argv[1], &servAdd.sin_addr) < 0 ){ //IP Address Connection
        printf("inet_pton() failure.\n");
        exit(2);
    }

    //Connect System Call
    if(connect(server, (struct sockaddr *) &servAdd,sizeof(servAdd))<0){//Connect()
        printf("connect() failure.\n");
        exit(3);
    }

    printf("Select any of the commands to run on the smain server: \n");
    printf("1. Upload a file to specific path. $ ufile <filename> <destination_path>\n");
    printf("2. Download a file from Server. $ dfile <filename>\n");
    printf("3. Delete file from server. $ rmfile <filename>\n");
    printf("4. Create and download tar file of .c .txt .pdf file types. $ dtar <file extesion>\n");
    printf("5. Display directory files on the server. $ display <directory_path>\n");
    printf("Please note 'file names' and 'paths' must be a tilde expansion path\n");

    while(1){ //Infinite loop start

        //Write message to the server
        char userCommand[1024];

        printf("Enter command: \n");
        fgets(userCommand, sizeof(userCommand), stdin);

        trimAndRemoveNewLine(userCommand); 
        printf("User Command: %s\n", userCommand);

        

        int argc = 0;
        char *argv[1024]; 
        parseInput(userCommand, argv, &argc);

        checkInput(argv, argc);

        //Converting argv into a single buffer
        char buffer[1024];
        buffer[0] = '\0';
        for (int i = 0; i < argc; i++) {
            strcat(buffer, argv[i]);  
            if (i < argc - 1) {
                strcat(buffer, " "); 
            }
        }

        //write(server, userCommand, strlen(userCommand) + 1); // Include null terminator in write
        write(server, buffer, strlen(buffer) + 1);

        //Read from pipe and display
        int bytes_read = read(server, message, MAXSIZE - 1);
        if (bytes_read < 0) {
            printf("Client: read() failure\n");
            exit(3);
        }
        message[bytes_read] = '\0';
        printf("Server: %s\n", message);

        
    } //Infinite loop end

    exit(0);
    return 0;
}