# Distributed File System Project

---

## Table of Contents
- [Introduction](#introduction)
- [Architecture](#architecture)
- [Client Commands](#client-commands)
- [Submission Instructions](#submission-instructions)
- [Usage](#usage)
- [License](#license)

---

## Introduction

This project implements a distributed file system using socket programming. The system consists of a main server (Smain) and two additional servers (Spdf and Stext) that handle different file types (.c, .pdf, .txt). Clients interact exclusively with Smain, which manages the storage and transfer of files to the appropriate servers.

---

## Architecture

The distributed file system is composed of the following components:

### Servers
- **Smain**: Main server that handles all client requests and stores .c files locally. It transfers .pdf and .txt files to Spdf and Stext servers respectively.
- **Spdf**: Server dedicated to storing .pdf files.
- **Stext**: Server dedicated to storing .txt files.

### Client (client24s)
Clients communicate with Smain to upload, download, remove files, and more without being aware of the other servers.

### File Storage Structure
- Files in Smain are saved under `~/smain`.
- Files in Stext are saved under `~/stext`.
- Files in Spdf are saved under `~/spdf`.

---

## Client Commands

The client process supports the following commands:

1. **ufile filename destination_path**: Uploads a file to Smain.
   - **File Types**: .c, .pdf, .txt
   - Example: `client24s$ ufile sample.c ~/smain/folder1/folder2`

2. **dfile filename**: Downloads a file from Smain to the client.
   - Example: `client24s$ dfile ~/smain/folder1/folder2/sample.txt`

3. **rmfile filename**: Deletes a file from Smain.
   - Example: `client24s$ rmfile ~/smain/folder1/folder2/sample.pdf`

4. **dtar filetype**: Creates a tar file of a specified type and downloads it.
   - Example: `client24s$ dtar pdf`

5. **display pathname**: Lists files in the specified directory.
   - Example: `client24s$ display ~/smain/folder1`

---

## Usage

1. **Setup**: Ensure all servers are running on different machines or terminals and are configured to communicate using sockets.
2. **Client Interaction**: Start the client and enter commands as specified above.

---

## License

This project is for educational purposes only. Please do not distribute or use for commercial purposes without permission.

---
