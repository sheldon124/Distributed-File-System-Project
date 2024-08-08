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
        char buffer[1024];
        char userCommand[1024];

        printf("Enter command: \n");
        fgets(userCommand, sizeof(userCommand), stdin);

        trimAndRemoveNewLine(userCommand); 
        printf("User Command: %s\n", userCommand);
        
        write(server, userCommand, strlen(userCommand) + 1); // Include null terminator in write


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