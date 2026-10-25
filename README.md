
# TFTP File Transfer Project

## Overview
A C implementation of a basic Trivial File Transfer Protocol (TFTP)
client and server using UDP sockets.

## Features
- Client-server communication over UDP
- Read request (RRQ) and write request (WRQ)
- DATA and ACK packet handling
- File reading and writing
- Modular code organization

## Files
- client.c: client operations
- server.c: server operations
- tftp.c / tftp.h: packet formation and parsing
- udp.c / udp.h: UDP communication
- file.c / file.h: file handling
- Makefile: build rules

## Build
```bash
make
```

## Run
Start the server from the directory containing the server executable:
```bash
mkdir -p server_files
cd server_files
echo "Hello TFTP" > sample.txt
../server 6969
```

From another terminal in the project root, download:
```bash
./client 127.0.0.1 6969 sample.txt downloaded.txt get
```

Upload:
```bash
echo "Upload test" > upload.txt
./client 127.0.0.1 6969 received.txt upload.txt put
```

## Limitations
This is an educational implementation and is not hardened for
untrusted networks. Test thoroughly before use.
