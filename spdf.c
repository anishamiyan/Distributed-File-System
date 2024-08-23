#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>


#define PORT 8092
#define BUF_SIZE 1024
//add your username here 
#define user "dogra7" 


void clientHandler(int cSocket);
void uploadFileHandler(int cSocket, const char *flNm);
void dtarFileHandler(int cSocket, const char *typeOfFile);
void clientFileTransferHandler(int cSocket, const char *flPth);


int main() {
   int sSocket, cSocket;
   struct sockaddr_in addrServer, addrClient;
   socklen_t addr_size = sizeof(addrClient);

   // Creation of socket
   if ((sSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
       printf("Socket creation failure\n");
       exit(1);
   }
   // Bind to port
   addrServer.sin_family = AF_INET;
   addrServer.sin_addr.s_addr = INADDR_ANY;
   addrServer.sin_port = htons(PORT);

   if (bind(sSocket, (struct sockaddr *)&addrServer, sizeof(addrServer)) < 0) {
       printf("Binding failure\n");
       close(sSocket);
       exit(1);
   }

   // Listen connections
   if (listen(sSocket, 3) < 0) {
       printf("Listening failure\n");
       close(sSocket);
       exit(1);
   }

   printf("Spdf server is running at port: %i\n", PORT);
   //accept connection
   while (1) {
       if ((cSocket = accept(sSocket, (struct sockaddr *)&addrClient, &addr_size)) < 0) {
           printf("Accept connection failure\n");
           continue;
       }
       // client connection handler
       clientHandler(cSocket);
       close(cSocket);
   }
   //close connection
   close(sSocket);
   return 0;
}

void clientHandler(int cSocket) {
   char pdfBfr[BUF_SIZE];
   char pdfCommand[BUF_SIZE];
   char flNm[BUF_SIZE];


   memset(pdfBfr, 0, BUF_SIZE);
   int szOfCmd = recv(cSocket, pdfBfr, BUF_SIZE, 0);


   if (szOfCmd <= 0) {
       if (szOfCmd == 0) {
           printf("client closed connection\n");
       } else {
           printf("Command not received\n");
       }
       return;
   }
   // Parsing file name and command
   sscanf(pdfBfr, "%s %s", pdfCommand, flNm);
  
   // command handler
   //ufile
   if (strcmp(pdfCommand, "ufile") == 0) {
       uploadFileHandler(cSocket, flNm);
   }
   //dtar
   else if (strcmp(pdfCommand, "dtar") == 0) {
        dtarFileHandler(cSocket, flNm);
   }
   //rmfile
   else if (strcmp(pdfCommand, "rmfile") == 0) {
    // mapping smain to spdf
    char completeFilePth[BUF_SIZE] = "/home/"user"/spdf/";
    //extracting file name from specified path
    strcat(completeFilePth, strrchr(flNm, '/') + 1); 
    if (remove(completeFilePth) == 0) {
        printf("File %s deleted successfully.\n", completeFilePth);
    } else {
        printf("Deletion of file failed\n");
    }
    }else {
       printf("command not known: %s\n", pdfCommand);
   }
}


//upload
void uploadFileHandler(int cSocket, const char *flNm) {
   char pdfBuffer[BUF_SIZE] = {0};
   char dstDir[BUF_SIZE] = "/home/"user"/spdf/"; 
   char completeFilePth[BUF_SIZE];


   // Extracting base file name from path provided
   const char *base_flNm = strrchr(flNm, '/');
   if (base_flNm) {
       base_flNm++;
   } else {
       base_flNm = flNm; 
   }

   // construction of full path 
   snprintf(completeFilePth, BUF_SIZE, "%s%s", dstDir, base_flNm);
   //create dir if not exist
   mkdir(dstDir, 0777); 

   FILE *fl = fopen(completeFilePth, "wb");
   if (!fl) {
       printf("File creation failed\n");
       return;
   }


   //storing contents
   int rdSize;
   while ((rdSize = recv(cSocket, pdfBuffer, BUF_SIZE, 0)) > 0) {
       printf(" Writing %d bytes to file\n", rdSize);
       fwrite(pdfBuffer, 1, rdSize, fl); 
       //clearing buffer after every write 
       memset(pdfBuffer, 0, BUF_SIZE); 
   }
   fclose(fl);
   printf("File stored at %s\n", completeFilePth);
}

//dtar
void dtarFileHandler(int cSocket, const char *typeOfFile) {
    char tar_name[BUF_SIZE];
    char cmd[BUF_SIZE];

    // tar file name creation
   snprintf(tar_name, BUF_SIZE, "%sfiles.tar", typeOfFile);  

    // Creation of tar file
    snprintf(cmd, BUF_SIZE, "tar -cvf %s -C /home/"user"/spdf .", tar_name);  
    int system_result = system(cmd);
    if (system_result != 0) {
        printf("error encountered creating tar file\n");
        return;
    }

    // checking successful creation of tar file
    if (access(tar_name, F_OK) == -1) {
        printf("Tar file creation failed\n");
        return;
    }
    // sending tar file to smain server
    clientFileTransferHandler(cSocket, tar_name);
}

void clientFileTransferHandler(int cSocket, const char *flPth) {
    FILE *file = fopen(flPth, "rb");
    if (!file) {
        printf("File not found\n");
        return;
    }

    char bfr[BUF_SIZE];
    size_t bRead;
    while ((bRead = fread(bfr, sizeof(char), BUF_SIZE, file)) > 0) {
        if (send(cSocket, bfr, bRead, 0) == -1) {
            printf("Sending file content failed\n");
        }
    }
    // signalling client file transfer is complete
    if (send(cSocket, "", 0, 0) == -1) {
        printf("Failure to send end of transmission signal\n");
    }
    fclose(file);
    printf("File sent to client: %s\n", flPth);
}





