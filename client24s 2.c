#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORT 8125
#define BUF_SIZE 1024

void handleFileUplds(int clntSockt, const char *nameOfFls, const char *destPaths);
void handleTarDwnlds(int clntSockt, const char *typeOfFl);
void handleListOfFls(int clntSockt, const char *nameOfPaths);
void handleRmCmd(int clntSockt, const char *inputCmds, const char *nameOfFls, const char *destPaths);
void handleDwnldFls(int clntSockt, const char *nameOfFls);
void sendInputCmds(int sock, const char *inputCmds, const char *nameOfFls, const char *destPaths);

int main() {
   int clntSockt;
   struct sockaddr_in addrOfServr;
   char inputCmds[BUF_SIZE], nameOfFls[BUF_SIZE], destPaths[BUF_SIZE];

   // Create socket
   if ((clntSockt = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
       printf("Creation of Socket failed");
       return -1;
   }

   addrOfServr.sin_family = AF_INET;
   addrOfServr.sin_port = htons(PORT);

   // Convert IPv4 and IPv6 addresses from text to binary form
   if (inet_pton(AF_INET, "127.0.0.1", &addrOfServr.sin_addr) <= 0) {
       printf("Address is invalid");
       close(clntSockt);
       return -1;
   }

   // Connect to the server
   if (connect(clntSockt, (struct sockaddr *)&addrOfServr, sizeof(addrOfServr)) < 0) {
       printf("Connection failed");
       close(clntSockt);
       return -1;
   }

   while (1) {
       printf("Enter inputCmds ufile/dfile/dtar/rmfile/display : ");
       fgets(inputCmds, BUF_SIZE, stdin);

       if (sscanf(inputCmds, "ufile %s %s", nameOfFls, destPaths) == 2) {
           handleFileUplds(clntSockt, nameOfFls, destPaths);
       } else if (sscanf(inputCmds, "dtar %s", nameOfFls) == 1) {
           handleTarDwnlds(clntSockt, nameOfFls);
       } else if (sscanf(inputCmds, "display %s", nameOfFls) == 1) {
           handleListOfFls(clntSockt, nameOfFls);
       }else if (sscanf(inputCmds, "rmfile %s", nameOfFls) == 1) {
            handleRmCmd(clntSockt, "rmfile", nameOfFls, "");
        }
        else if (sscanf(inputCmds, "dfile %s", nameOfFls) == 1) {
            handleDwnldFls(clntSockt, nameOfFls);
        }
        else {
           printf("Please pass valid input command\n");
       }
   }

   close(clntSockt);
   return 0;
}

void sendInputCmds(int sock, const char *inputCmds, const char *nameOfFls, const char *destPaths) {
   char buffer[BUF_SIZE];

   if (strcmp(inputCmds, "dfile") == 0) {
       // For dfile, don't include destPaths
       snprintf(buffer, BUF_SIZE, "%s %s", inputCmds, nameOfFls);
   } 
   else {
       snprintf(buffer, BUF_SIZE, "%s %s %s", inputCmds, nameOfFls, destPaths);
   }

   if (send(sock, buffer, strlen(buffer), 0) != strlen(buffer)) {
       perror("Failed to send the complete inputCmds");
   } 
   else {
       printf("inputCmds sent successfully: %s\n", buffer);
   }
}

void handleFileUplds(int sock, const char *nameOfFls, const char *destPaths) {
   // Send the inputCmds and nameOfFls first
   sendInputCmds(sock, "ufile", nameOfFls, destPaths);

   // Clear the buffer before reading the file
   char buffer[BUF_SIZE] = {0};

   // Open the file and send its contents
   FILE *file = fopen(nameOfFls, "rb");
   if (!file) {
       //perror("File not found");
       return;
   }

   size_t bytes_read;
   while ((bytes_read = fread(buffer, 1, BUF_SIZE, file)) > 0) {
       printf("Sending %zu bytes of file content\n", bytes_read);
       if (send(sock, buffer, bytes_read, 0) == -1) {
           perror("Failed to send file content");
           break;
       }
        // Clear the buffer after each send
       memset(buffer, 0, BUF_SIZE); 
   }

   fclose(file);
   printf("File uploaded successfully\n");

   // Optionally, send a zero-length packet to signal end of transmission
   send(sock, "", 0, 0);
}

void handleTarDwnlds(int sock, const char *file_type) {
   char buffer[BUF_SIZE];
    char tar_name[BUF_SIZE];

    // Send the dtar command to the server
    snprintf(buffer, BUF_SIZE, "dtar %s", file_type);
    send(sock, buffer, strlen(buffer), 0);
    printf("Tar file downloaded successfully..\n");

    // Open the file to write the downloaded content
    FILE *file = fopen(tar_name, "wb");
    if (!file) {
        //perror("File creation failed");
        return;
    }

    
    int bytes_read;
    while ((bytes_read = recv(sock, buffer, BUF_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_read, file);

        // If we receive a zero-length packet, break out of the loop
        if (bytes_read == 0) {
            break;
        }
        break;
    }

    fclose(file);
}

void handleListOfFls(int sock, const char *nameOfPaths) {
    char handleListBuffer[BUF_SIZE];
    int totalBytesReceived;

    // Send the listfiles inputCmds to the server
    snprintf(handleListBuffer, BUF_SIZE, "display %s", nameOfPaths);
    send(sock, handleListBuffer, strlen(handleListBuffer), 0);

    printf("List of files:\n");

    // Continuously receive data from the server
    while ((totalBytesReceived = recv(sock, handleListBuffer, BUF_SIZE - 1, 0)) > 0) {
        handleListBuffer[totalBytesReceived] = '\0';  // Null-terminate the buffer

        // Check for the end-of-transmission marker
        if (strcmp(handleListBuffer, "END_OF_LIST") == 0) {
            // Break the loop if the end marker is received
            break;  
        }
        // Print the received data
        printf("%s", handleListBuffer);  
    }

    if (totalBytesReceived <= 0 && strcmp(handleListBuffer, "END_OF_LIST") != 0) {
        printf("\nError or disconnection while receiving file list.\n");
    } else {
       printf("");
    }
}

void handleRmCmd(int clientSock, const char *inputCmds, const char *nameOfFls, const char *destPaths) {
   char rmCmdbuffer[BUF_SIZE];
  
   if (strcmp(inputCmds, "dfile") == 0) {
       // For dfile, don't include destPaths
       snprintf(rmCmdbuffer, BUF_SIZE, "%s %s", inputCmds, nameOfFls);
   } else {
       snprintf(rmCmdbuffer, BUF_SIZE, "%s %s %s", inputCmds, nameOfFls, destPaths);
   }
   printf("File removed..");
   if (send(clientSock, rmCmdbuffer, strlen(rmCmdbuffer), 0) != strlen(rmCmdbuffer)) {
       perror("Failed to send the complete inputCmds");
   } else {
       printf("inputCmds sent successfully: %s\n", rmCmdbuffer);
   }
}

void handleDwnldFls(int clientSock, const char *nameOfFls) {
     char dwnldBuffer[BUF_SIZE];

    // Send the dfile command to the server
    snprintf(dwnldBuffer, BUF_SIZE, "dfile %s", nameOfFls);
    send(clientSock, dwnldBuffer, strlen(dwnldBuffer), 0);

    // Extract the filename from the full path
    const char *baseFilename = strrchr(nameOfFls, '/');
    if (baseFilename) {
        baseFilename++;  
    } 
    else {
        baseFilename = nameOfFls;  
    }

    // Open the file to write the downloaded content
    FILE *myFile = fopen(baseFilename, "wb");
    if (!myFile) {
        perror("Failed to create file");
        return;
    }

    // Receive the file contents from the server and write to file
    int totalBytesRead;
    size_t totalBytesWrtn = 0;

    while ((totalBytesRead = recv(clientSock, dwnldBuffer, BUF_SIZE, 0)) > 0) {
        printf("Received %d bytes from server\n", totalBytesRead);
        fwrite(dwnldBuffer, 1, totalBytesRead, myFile);
         // flushing the buffer after each write
        fflush(myFile); 
        totalBytesWrtn += totalBytesRead;
        printf("Written %d bytes to file %s\n", totalBytesRead, baseFilename);
    }

    if (totalBytesRead < 0) {
        perror("Error receiving file content");
    } else {
        printf("File %s downloaded successfully with %zu bytes.\n", baseFilename, totalBytesWrtn);
    }

    fclose(myFile);
}