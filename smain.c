#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>

#define PORT 8125
#define BUF_SIZE 1024


void handleClient(int cSocket);
void shareContentToClient(int cSocket, const char *file_path);
void saveContentInLocal(const char *fileNm, const char *dstDir, int cSocket);
void shareContentToServer(const char *fileNm, const char *ipOfServer, int portOfServer, const char *inputCmd);
void dtarFunctionHandler(int cSocket, const char *file_type);
void makeTarFile(const char *file_type, const char *nameOfTarFile, const char *directory);
void displayFiles(int cSocket, const char *directory);
void transferDfileText(const char *ipOfServer, int portOfServer, const char *inputCmd);
void whatToDownload(int cSocket, const char *file_path);
void rmSpecifiedFile(const char *fileNm, const char *displayBuff);
void dfileFunc(int cSocket, const char *fileNm);

int main() {
    int sSocket, cSocket;
    struct sockaddr_in addrServer, addrClient;
    socklen_t addr_size = sizeof(addrClient);

    // socket creation
    if ((sSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        printf("Creation of socket failed\n");
        exit(1);
    }

    // port binding
    addrServer.sin_family = AF_INET;
    addrServer.sin_addr.s_addr = INADDR_ANY;
    addrServer.sin_port = htons(PORT);

    if (bind(sSocket, (struct sockaddr *)&addrServer, sizeof(addrServer)) < 0) {
        printf("Binding failure\n");
        close(sSocket);
        exit(1);
    }

    // Listen for connection
    if (listen(sSocket, 3) < 0) {
        printf("Listening failure\n");
        close(sSocket);
        exit(1);
    }

    printf("Smain server is running at PORT NUMBER %i and waiting for connections...\n", PORT);

    while (1) {
        // Connection acceptance
        if ((cSocket = accept(sSocket, (struct sockaddr *)&addrClient, &addr_size)) < 0) {
            printf("Accept failed\n");
            continue;
        }

        // Fork to handle client
        if (fork() == 0) {
            close(sSocket);
            handleClient(cSocket);
            close(cSocket);
            exit(0);
        } else {
            close(cSocket);
        }

        // Clean up zombie processes
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

    close(sSocket);
    return 0;
}

void handleClient(int cSocket) {
    char displayBuff[BUF_SIZE];
    char inputCmd[BUF_SIZE], fileNm[BUF_SIZE], dstPath[BUF_SIZE];

    while (1) {
        memset(displayBuff, 0, BUF_SIZE);
        int rd = recv(cSocket, displayBuff, BUF_SIZE, 0);
        if (rd <= 0) {
            break;
        }

        sscanf(displayBuff, "%s %s %s", inputCmd, fileNm, dstPath);

        if (strcmp(inputCmd, "ufile") == 0) {
            // Handle different file types
            char *extOfFile = strrchr(fileNm, '.');
            if (extOfFile == NULL) {
                printf("No file extension found.\n");
                continue;
            }

            if (strcmp(extOfFile, ".c") == 0) {
                // Handle .c files locally
                saveContentInLocal(fileNm, dstPath, cSocket);
            } 
            else if (strcmp(extOfFile, ".txt") == 0) {
                // Send .txt file to Stext server
                shareContentToServer(fileNm, "127.0.0.4", 8095, displayBuff);
            } 
            else if (strcmp(extOfFile, ".pdf") == 0) {
                // Send .pdf file to Spdf server
                shareContentToServer(fileNm, "127.0.0.4", 8092, displayBuff);
            }
        } 
        else if (strcmp(inputCmd, "dtar") == 0) {
            dtarFunctionHandler(cSocket, fileNm);
        } 
        else if (strcmp(inputCmd, "display") == 0) {
            displayFiles(cSocket, fileNm);
        }
        else if (strcmp(inputCmd, "rmfile") == 0) {
            rmSpecifiedFile(fileNm,displayBuff);
        }
        else if (strcmp(inputCmd, "dfile") == 0) {
            dfileFunc(cSocket, fileNm);
        }
    }
}


void rmSpecifiedFile(const char *fileNm, const char *displayBuff) {
    // Handle rmfile inputCmd
        char *extOfFile = strrchr(fileNm, '.');

        if (strcmp(extOfFile, ".c") == 0) {
            //.c files
            remove(fileNm); 
            printf("File %s deleted in local.\n", fileNm);
        } 
        else if (strcmp(extOfFile, ".txt") == 0) {
            // handling .txt files
            shareContentToServer(fileNm, "127.0.0.4", 8095, displayBuff);
        } 
        else if (strcmp(extOfFile, ".pdf") == 0) {
            // handling .pdf files
            shareContentToServer(fileNm, "127.0.0.4", 8092, displayBuff);
        }
}

void saveContentInLocal(const char *fileNm, const char *dstDir, int cSocket) {
    char displayBuff[BUF_SIZE];
    char completeFilePth[BUF_SIZE];

    //full path creation
    snprintf(completeFilePth, BUF_SIZE, "%s/%s", dstDir, strrchr(fileNm, '/') + 1);

    // destination directory creation
    mkdir(dstDir, 0777); 
    FILE *fl = fopen(completeFilePth, "wb");
    if (!fl) {
        printf("creation of file failed\n");
        return;
    }

    //get data from client and writing to file
    ssize_t rd;
    //read
    while ((rd = recv(cSocket, displayBuff, BUF_SIZE, 0)) > 0) {
        printf("Writing %zd bytes to file\n", rd);
        // Write 
        fwrite(displayBuff, 1, rd, fl); 
        // Flush displayBuffer
        fflush(fl);
    }

    if (rd < 0) {
        printf("Error getting file data\n");
    } else {
        printf("Completed file storage at %s.\n", completeFilePth);
    }

    fclose(fl);
}

void shareContentToServer(const char *fileNm, const char *ipOfServer, int portOfServer, const char *inputCmd) {
    int sckt;
    struct sockaddr_in addrServer;
    char displayBuff[BUF_SIZE];
    FILE *file;

    printf("Attempting to transfer file to server %s:%d\n", ipOfServer, portOfServer);

    // Create socket
    if ((sckt = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return;
    }

    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(portOfServer);

    // converting text to binary of port address
    if (inet_pton(AF_INET, ipOfServer, &addrServer.sin_addr) <= 0) {
        printf("Invalid address\n");
        close(sckt);
        return;
    }

    // Connect to the server
    if (connect(sckt, (struct sockaddr *)&addrServer, sizeof(addrServer)) < 0) {
        printf("Connection failed\n");
        close(sckt);
        return;
    }

    printf("server connected %s:%d\n", ipOfServer, portOfServer);

    // Sending cmd to target server
    if (send(sckt, inputCmd, strlen(inputCmd), 0) == -1) {
        perror("inputCmd failed to server");
        close(sckt);
        return;
    }

    // Send the fileNm to the target server
    if (send(sckt, fileNm, strlen(fileNm), 0) == -1) {
        perror("Sending fileNm failed");
        close(sckt);
        return;
    }

    printf("fileNm %s sent to server %s:%d\n", fileNm, ipOfServer, portOfServer);

    // Open the file for reading
    file = fopen(fileNm, "rb");
    if (!file) {
        close(sckt);
        return;
    }

    // Send the file contents
    ssize_t bytesToRead;
    while ((bytesToRead = fread(displayBuff, sizeof(char), BUF_SIZE, file)) > 0) {
        if (send(sckt, displayBuff, bytesToRead, 0) == -1) {
            printf("Sending file content failed");
            break;
        }
    }

    // Ensure the displayBuff is flushed and the end of the content is properly sent
    send(sckt, "", 0, 0);

    fclose(file);
    close(sckt);
    printf("Sucessfully transferred to server %s:%d\n", ipOfServer, portOfServer);
}

void shareContentToClient(int cSocket, const char *file_path) {
    FILE *shareContentFile = fopen(file_path, "rb");
    if (!shareContentFile) {
        return;
    }

    char filedisplayBuff[BUF_SIZE];
    size_t bytesToRead;
    while ((bytesToRead = fread(filedisplayBuff, sizeof(char), BUF_SIZE, shareContentFile)) > 0) {
        send(cSocket, filedisplayBuff, bytesToRead, 0);
    }

    // Signal the client that the file transfer is complete
    send(cSocket, "", 0, 0);  // Send a zero-length message

    fclose(shareContentFile);
    printf("Sent the files to client: %s\n", file_path);
}

void dtarFunctionHandler(int cSocket, const char *file_type) {
    char nameOfTarFile[BUF_SIZE];

    // Correct tar file names and requests
    if (strcmp(file_type, "c") == 0) {
        snprintf(nameOfTarFile, BUF_SIZE, "cfiles.tar");
        makeTarFile(".c", nameOfTarFile, "/home/dogra7/smain/folder1");
    } 
    else if (strcmp(file_type, "pdf") == 0) {
        snprintf(nameOfTarFile, BUF_SIZE, "pdf");
        shareContentToServer(nameOfTarFile, "127.0.0.4", 8092, "dtar pdf");
    } 
    else if (strcmp(file_type, "txt") == 0) {
        snprintf(nameOfTarFile, BUF_SIZE, "text");
        shareContentToServer(nameOfTarFile, "127.0.0.4", 8095, "dtar txt");
    } 
    else {
        printf("Unsupported file type for dtar: %s\n", file_type);
        return;
    }

    // Allow time for the tar file to be created
    sleep(1); 
    shareContentToClient(cSocket, nameOfTarFile);
}

void makeTarFile(const char *file_type, const char *nameOfTarFile, const char *directory) {
    char inputCmd[BUF_SIZE];

    // Create the tar file by directly referencing the files
    snprintf(inputCmd, BUF_SIZE, "tar -cvf %s -C %s .", nameOfTarFile, directory);  
    //printf("Debug: Running inputCmd - %s\n", inputCmd);
    int result = system(inputCmd);

    if (result != 0) {
        printf("Tar failed\n");
    }
}

void displayFiles(int cSocket, const char *directory) {
    DIR *dir;
    struct dirent *entry;
    struct stat statOfFl;
    char displayBuff[BUF_SIZE];
    char pathOfFl[BUF_SIZE];

    const char *directories[] = {"/home/dogra7/smain/folder1", "/home/dogra7/spdf", "/home/dogra7/stext"};
    int numbrOfDir = 3;

    for (int i = 0; i < numbrOfDir; i++) {
        if ((dir = opendir(directories[i])) == NULL) {
            printf("opendir failure");
            continue;
        }

        while ((entry = readdir(dir)) != NULL) {
            snprintf(pathOfFl, sizeof(pathOfFl), "%s/%s", directories[i], entry->d_name);
            if (stat(pathOfFl, &statOfFl) == 0 && S_ISREG(statOfFl.st_mode)) {
                // full path of the file
                snprintf(displayBuff, BUF_SIZE, "%s\n", pathOfFl);  
                send(cSocket, displayBuff, strlen(displayBuff), 0);
            }
        }
        closedir(dir);
    }

    // Send the end-of-list marker
    strcpy(displayBuff, "END_OF_LIST");
    send(cSocket, displayBuff, strlen(displayBuff), 0);
}

//dfile ->
void dfileFunc(int cSocket, const char *fileNm) {
    char *extOfFile = strrchr(fileNm, '.');
    if (extOfFile == NULL) {
        printf("No file extension found.\n");
        return;
    }

    if (strcmp(extOfFile, ".c") == 0) {
        // Handle .c files locally
        whatToDownload(cSocket, fileNm);
    } 
    else if (strcmp(extOfFile, ".txt") == 0) {
        // Request .txt file from Stext server
        char inputCmd[BUF_SIZE];
        snprintf(inputCmd, BUF_SIZE, "dfile %s", fileNm);
        transferDfileText( "127.0.0.4", 8095, inputCmd);
    } 
    else if (strcmp(extOfFile, ".pdf") == 0) {
        // Request .pdf file from Spdf server
        char inputCmd[BUF_SIZE];
        snprintf(inputCmd, BUF_SIZE, "dfile %s", fileNm);
        shareContentToServer(inputCmd, "127.0.0.4", 8092, inputCmd);
    } else {
        printf("Unsupported file extension: %s\n", extOfFile);
    }
}

void whatToDownload(int cSocket, const char *pathOfDownloadFl) {
    char completeFilePth[BUF_SIZE];
    const char *extOfFile = strrchr(pathOfDownloadFl, '.');

    if (extOfFile == NULL) {
        printf("No file extension found.\n");
        return;
    }

    // Determine the base directory based on the file extension
    if (strcmp(extOfFile, ".c") == 0) {
        snprintf(completeFilePth, BUF_SIZE, "/home/dogra7/smain/folder1/%s", basename(pathOfDownloadFl));
    } 
    else if (strcmp(extOfFile, ".txt") == 0) {
        snprintf(completeFilePth, BUF_SIZE, "/home/dogra7/stext/%s", basename(pathOfDownloadFl));
    } 
    else if (strcmp(extOfFile, ".pdf") == 0) {
        snprintf(completeFilePth, BUF_SIZE, "/home/dogra7/spdf/%s", basename(pathOfDownloadFl));
    } 
    else {
        printf("Unsupported file extension: %s\n", extOfFile);
        return;
    }

    printf("Debug: Full path to send: %s\n", completeFilePth);

    FILE *file = fopen(completeFilePth, "rb");
    if (!file) {
        perror("File not found");
        return;
    }

    char displayBuff[BUF_SIZE];
    size_t bytesToRead;

    while ((bytesToRead = fread(displayBuff, 1, BUF_SIZE, file)) > 0) {
        printf("Read %zu bytes from file %s\n", bytesToRead, completeFilePth);

        if (send(cSocket, displayBuff, bytesToRead, 0) == -1) {
            perror("Failed to send file content");
            fclose(file);
            return;
        }
        printf("Sent %zu bytes to client\n", bytesToRead);
    }

    if (ferror(file)) {
        printf("Reading of file failed");
    } else {
        printf("File sent to client: %s\n", completeFilePth);
    }

    fclose(file);
}

void transferDfileText(const char *ipOfServer, int portOfServer, const char *inputCmd) {
    int transferSock;
    struct sockaddr_in addrServer;
    char displayBuff[BUF_SIZE];
    ssize_t bytesToRead;

    // Create socket
    if ((transferSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error");
        return;
    }

    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(portOfServer);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ipOfServer, &addrServer.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported");
        close(transferSock);
        return;
    }

    // Connect to the Stext server
    if (connect(transferSock, (struct sockaddr *)&addrServer, sizeof(addrServer)) < 0) {
        printf("Connection failed");
        close(transferSock);
        return;
    }

    // Send the inputCmd to the Stext server
    if (send(transferSock, inputCmd, strlen(inputCmd), 0) == -1) {
        printf("inputCmd failed");
        close(transferSock);
        return;
    }

    // Receive the file content from the Stext server and send it to the client
    while ((bytesToRead = recv(transferSock, displayBuff, BUF_SIZE, 0)) > 0) {
        if (send(transferSock, displayBuff, bytesToRead, 0) == -1) {
            printf("Forwarding file content to client failed");
            break;
        }
    }

    if (bytesToRead < 0) {
        printf("Error getting the file content from Stext\n");
    } else {
        printf("File content successfully shared to client.\n");
    }

    close(transferSock);
}
