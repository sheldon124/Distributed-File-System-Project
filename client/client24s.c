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

    while(1){ //Infinite loop start

        //Write message to the server
        char buffer[1024];
        printf("Enter the message to be sent to the server: \n");
        //scanf("%s", &buffer);
        fgets(buffer, sizeof(buffer), stdin);
        //scanf("%1023s", buffer); // Length validation
        
        // Remove newline character if present
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        write(server, buffer, strlen(buffer) + 1); // Include null terminator in write


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