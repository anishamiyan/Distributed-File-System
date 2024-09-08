# Distributed File System

This project implements a Distributed File System using socket programming, allowing multiple clients to upload, download, delete, and manage files through a centralized main server (`Smain`). The system is designed with three servers—`Smain`, `Spdf`, and `Stext`—each responsible for handling specific file types (.c, .pdf, .txt), ensuring efficient file management across the network.

## Features

### Multiple Server Support

- **Smain**: Main server that handles `.c` files locally and delegates `.pdf` and `.txt` files to `Spdf` and `Stext` servers respectively.
- **Spdf**: Dedicated server for storing `.pdf` files.
- **Stext**: Dedicated server for storing `.txt` files.

### Client Commands

Clients can interact with the Distributed File System using the following commands:

- `ufile filename destination_path`: Uploads a file to `Smain`, which delegates it to the appropriate server if necessary.
- `dfile filename`: Downloads a file from `Smain` or the designated server.
- `rmfile filename`: Deletes a file from `Smain` or the designated server.
- `dtar filetype`: Creates and downloads a tar file containing all files of a specific type (.c, .pdf, or .txt).
- `display pathname`: Displays a list of all `.c`, `.pdf`, and `.txt` files in a specified directory.

## File Structure

- **Smain.c**: The main server code that handles `.c` files locally and coordinates with `Spdf` and `Stext` for `.pdf` and `.txt` files.
- **Spdf.c**: Server code dedicated to handling `.pdf` files.
- **Stext.c**: Server code dedicated to handling `.txt` files.
- **client24s.c**: Client-side code that allows users to interact with the distributed file system.


