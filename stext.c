#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <dirent.h>  

#define PORT 8095
#define BUF_SIZE 1024
#define user "dogra7" 

void clientHandler(int cSocket);
void ufileHandler(int cSocket, const char *nameOfFl);
void dtarHandler(int cSocket, const char *typeOfFl);
void sendFilesToClient(int cSocket, const char *pathOfFl);
void listOfFilesHandler(int cSocket, const char *dirOfFile);
void sendfilesToDownload(int cSocket, const char *nameOfFl);

int main() {
   int sSocket, cSocket;
   struct sockaddr_in addrOfServer, addrOfClient;
   socklen_t addr_size = sizeof(addrOfClient);


   // Creating socket
   if ((sSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
       printf("Socket failed");
       exit(1);
   }


   // Binding to the port
   addrOfServer.sin_family = AF_INET;
   addrOfServer.sin_addr.s_addr = INADDR_ANY;
   addrOfServer.sin_port = htons(PORT);

   //handler error condition of bind
   if (bind(sSocket, (struct sockaddr *)&addrOfServer, sizeof(addrOfServer)) < 0) {
       printf("Bind failed");
       close(sSocket);
       exit(1);
   }


   // Listening for connections
   if (listen(sSocket, 3) < 0) {
       printf("Listen failed");
       close(sSocket);
       exit(1);
   }

   printf("Stext server is running at PORT NUMBER: %i\n", PORT);

   while (1) {
       if ((cSocket = accept(sSocket, (struct sockaddr *)&addrOfClient, &addr_size)) < 0) {
           printf("Accept failed");
           continue;
       }
       // Handle client connection
       clientHandler(cSocket);
       close(cSocket);
   }
   close(sSocket);
   return 0;
}


//handle client connection
void clientHandler(int cSocket) {
   char textFileBuffer[BUF_SIZE];
   char textCommand[BUF_SIZE];
   char nameOfFl[BUF_SIZE];

   memset(textFileBuffer, 0, BUF_SIZE);
   int textCommand_size = recv(cSocket, textFileBuffer, BUF_SIZE, 0);

   if (textCommand_size <= 0) {
       if (textCommand_size == 0) {
           printf("Connection closed by client.\n");
       } else {
           printf("Failed to receive the textCommand");
       }
       return;
   }

   // Parse the command and nameOfFl
   sscanf(textFileBuffer, "%s %s", textCommand, nameOfFl);
   printf("Received textFileBuffer: %s\n", textFileBuffer);


   // Handle the appropriate command
   if (strcmp(textCommand, "ufile") == 0) {
       ufileHandler(cSocket, nameOfFl);
   } 
   else if (strcmp(textCommand, "dtar") == 0) {
        dtarHandler(cSocket, nameOfFl);
   }
   else if (strcmp(textCommand, "listfiles") == 0) {
        listOfFilesHandler(cSocket, nameOfFl);
    }
    else if (strcmp(textCommand, "rmfile") == 0) {
    // Map the smain path to the stext dirOfFile
    char full_path[BUF_SIZE] = "/home/"user"/stext/";
    strcat(full_path, strrchr(nameOfFl, '/') + 1); 

        // Attempt to delete the file in the stext dirOfFile
        if (remove(full_path) == 0) {
            printf("File %s deleted successfully.\n", full_path);
        } 
        else {
        printf("Failed to delete the file");
        }
    }
    else if (strcmp(textCommand, "dfile") == 0) {
        char *space_pos = strchr(textFileBuffer, ' ');
    if (space_pos != NULL) {
        *space_pos = '\0'; 
        strcpy(textCommand, textFileBuffer);
        strcpy(nameOfFl, space_pos + 1);  
    } 
    else {
        // Handle case where no space is found (error scenario)
        strcpy(textCommand, textFileBuffer);
        nameOfFl[0] = '\0';
    }
        sendfilesToDownload(cSocket, nameOfFl);
    }


   else {
       printf("Command unknow: %s\n", textCommand);
   }
}

//handle upload file
void ufileHandler(int cSocket, const char *nameOfFl) {
   char textFileBuffer[BUF_SIZE] = {0};
   char destination_dir[BUF_SIZE] = "/home/"user"/stext/";
   char full_path[BUF_SIZE];


   // Extract the base nameOfFl from the full path
   const char *base_nameOfFl = strrchr(nameOfFl, '/');
   if (base_nameOfFl) {
    // Move past the last '/'
    base_nameOfFl++;  
   } 
   else {
    base_nameOfFl = nameOfFl;  
   }


   // Construct the full path for the file within the appropriate dirOfFile
   snprintf(full_path, BUF_SIZE, "%s%s", destination_dir, base_nameOfFl);
   printf("Storing file at - %s\n", full_path);


   // Ensure the directory exists
   mkdir(destination_dir, 0777); 


   // Open the file for writing in binary mode
   FILE *file = fopen(full_path, "wb");
   if (!file) {
       printf("File creation failed");
       return;
   }


   // Receive and store the file contents
   int readSize;
   while ((readSize = recv(cSocket, textFileBuffer, BUF_SIZE, 0)) > 0) {
       printf("Debug: Writing %d bytes to file\n", readSize);
       fwrite(textFileBuffer, 1, readSize, file);  
       memset(textFileBuffer, 0, BUF_SIZE);  
   }


   fclose(file);
   printf("File received and stored at %s\n", full_path);
}

//handle tar file
void dtarHandler(int cSocket, const char *typeOfFl) {
    char tarFileName[BUF_SIZE];
    char textCommand[BUF_SIZE];

    // Set the tar file name based on the file type
    snprintf(tarFileName, BUF_SIZE, "%sfiles.tar", typeOfFl);  
    // Create the tar file containing all files in the dirOfFile
    snprintf(textCommand, BUF_SIZE, "tar -cvf %s -C /home/"user"/stext .", tarFileName); 
    
    // Execute the command to create the tar file
    int systemResult = system(textCommand);

    if (systemResult != 0) {
        printf("Error: Tar command failed\n");
        return;
    }

    // Check if the tar file was created successfully
    if (access(tarFileName, F_OK) == -1) {
        printf("Tar file creation failed");
        return;
    }

    // Send the tar file to the Smain server
    sendFilesToClient(cSocket, tarFileName);
}


void sendFilesToClient(int cSocket, const char *pathOfFl) {
    FILE *file = fopen(pathOfFl, "rb");
    if (!file) {
        printf("File not found");
        return;
    }

    //read bytes
    char textFileBuffer[BUF_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(textFileBuffer, sizeof(char), BUF_SIZE, file)) > 0) {
        if (send(cSocket, textFileBuffer, bytes_read, 0) == -1) {
            printf("Failed to send file content");
        }
    }

    fclose(file);
    printf("File sent to client: %s\n", pathOfFl);
}

//display files
void listOfFilesHandler(int cSocket, const char *dirOfFile) {
    DIR *fileDir;
    struct dirent *entry;
    struct stat file_stat;
    char textFileBuffer[BUF_SIZE];
    char filepath[BUF_SIZE];

    if ((fileDir = opendir(dirOfFile)) == NULL) {
        printf("opening directory error");
        return;
    }

    //read name of files
    while ((entry = readdir(fileDir)) != NULL) {
        snprintf(filepath, sizeof(filepath), "%s/%s", dirOfFile, entry->d_name);
        if (stat(filepath, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            snprintf(textFileBuffer, BUF_SIZE, "%s\n", entry->d_name);
            send(cSocket, textFileBuffer, strlen(textFileBuffer), 0);
        }
    }
    closedir(fileDir);

    // Send the end-of-list marker
    strcpy(textFileBuffer, "END_OF_LIST");
    send(cSocket, textFileBuffer, strlen(textFileBuffer), 0);
}

void sendfilesToDownload(int cSocket, const char *nameOfFl) {
    char fullFilePath[BUF_SIZE];
    snprintf(fullFilePath, BUF_SIZE, "/home/"user"/stext/%s", basename(nameOfFl));

    FILE *txtFile = fopen(fullFilePath, "rb");
    if (!txtFile) {
        printf("File not found on Stext server");
        return;
    }

    char textFileBuffer[BUF_SIZE];
    size_t checkBytesRead;

    while ((checkBytesRead = fread(textFileBuffer, 1, BUF_SIZE, txtFile)) > 0) {
        send(cSocket, textFileBuffer, checkBytesRead, 0);
    }

    fclose(txtFile);
    printf("File %s sent to Smain.\n", fullFilePath);
}


