# How to execute

## 1. metasrv
Metasrv give ip addrs and port numbers when client sends a request to metasrv.
- First, when metasrv is opened, metasrv waits client request.
- When client is opened and Mount Function is executed, client request server informations (ip addrs and port numbers) to metasrv. 
- When metasrv get Mount request, metasrv send ip addr list and port number list to client according to a config files
- config file: 1. config_file_two.txt: in case of two servers, 2. config_file.txt: in case of five servers

Usage: ./nanafs_metasrv <port number>
```c
$ ./nanafs_metasrv 50070
Meta Server listening on 127.0.0.1:50070
```

## 2. server
Servers intaract with client when open, read, close syscall called
- First, server waits client request
- when open called, client and server interact for the first time
- now 2 - 5 servers can be opened and interact with one client

Usage: ./nanafs_server <IP address> <port number>
- when using two server
- cannot specify ip addr
- now using 0.0.0.0 ip addr
```c
$ ./nanafs_server 0.0.0.0 50059
Server listening on 0.0.0.0:50059
```
```c
$ ./nanafs_server 0.0.0.0 50056
Server listening on 0.0.0.0:50056
```

## 3. client
First, client interact with metasrv and get server information (ip addrs and port numbers)
Then, client send request to servers when open, read, close syscall are called by user(file operation)
Client can set timeout
However, the behavior when timeout occur isnot defined

Usage: ./nanafs_client <mount directory> -d
- -d: the option which show output comments of fuse
- in the first execution, user must make the mount directory

```c
$ mkdir ho4
$ ./nanafs_client ho4 -d
FUSE library version: 3.10.5
nullpath_ok: 0
unique: 2, opcode: INIT (26), nodeid: 0, insize: 56, pid: 0
INIT: 7.34
flags=0x33fffffb
max_readahead=0x00020000
0;ip: 192.168.10.1, port: 5056, server_channel: 192.168.10.1:5056
1;ip: 192.168.10.2, port: 5059, server_channel: 192.168.10.2:5059
stubs num: 2
   INIT: 7.31
   flags=0x0040f039
   max_readahead=0x00020000
   max_write=0x00100000
   max_background=0
   congestion_threshold=0
   time_gran=1
   unique: 2, success, outsize: 80
```

## 4. user (file operation)
Lastly, execute test_fuce.c file which open, read and close txt file in mounted directory
```c
$ gcc test_fuse.c
$ ./a.out
```

# Execution Flow
First, open metasrv
```c
$ ./nanafs_metasrv 50070
Meta Server listening on 127.0.0.1:50070
```

Second, open several servers
```c
$ ./nanafs_server 0.0.0.0 50059
Server listening on 0.0.0.0:50059
```
```c
$ ./nanafs_server 0.0.0.0 50056
Server listening on 0.0.0.0:50056
```

Third, open client
```c
$ ./nanafs_client ho4 -d
```

Then, client connect to metasrv and get server's informations

```c
$ ./nanafs_metasrv 50070
Meta Server listening on 127.0.0.1:50070
ip list:192.168.10.1
192.168.10.2

port list:5056
5059

server_num: 2
```
```c
$ ./nanafs_client ho4 -d
FUSE library version: 3.10.5
nullpath_ok: 0
unique: 2, opcode: INIT (26), nodeid: 0, insize: 56, pid: 0
INIT: 7.34
flags=0x33fffffb
max_readahead=0x00020000
stubs num: 2
   INIT: 7.31
   flags=0x0040f039
   max_readahead=0x00020000
   max_write=0x00100000
   max_background=0
   congestion_threshold=0
   time_gran=1
   unique: 2, success, outsize: 80
```

After that, execute user file which execute file operation (open, read, close)
```c
$ gcc test_fuse.c
$ ./a.out
```
```c
$ ./nanafs_server 0.0.0.0 50059
Server listening on 0.0.0.0:50059
Open(hello-world.txt.0): fd=10
Open(dead-beef.txt.0): fd=11
Open(abc-def.txt.0): fd=12
sleep for 10 sec
sleep for 10 sec
sleep for 10 sec
sleep finished
Read(fd=10, count=8192, offset=0): len=5, size: 5
sleep finished
Read(fd=11, count=8192, offset=0): len=4, size: 4
sleep finished
Read(fd=12, count=8192, offset=0): len=3, size: 3
```
```c
$ ./nanafs_server 0.0.0.0 50056
Server listening on 0.0.0.0:50056
Open(hello-world.txt.1): fd=10
Open(dead-beef.txt.1): fd=11
Open(abc-def.txt.1): fd=12
sleep for 10 sec
sleep for 10 sec
sleep for 10 sec
sleep finished
Read(fd=10, count=8192, offset=0): len=7, size: 7
sleep finished
Read(fd=11, count=8192, offset=0): len=5, size: 5
sleep finished
Read(fd=12, count=8192, offset=0): len=4, size: 4
```
```c
$ ./nanafs_client ho4 -d
FUSE library version: 3.10.5
nullpath_ok: 0
unique: 2, opcode: INIT (26), nodeid: 0, insize: 56, pid: 0
INIT: 7.34
flags=0x33fffffb
max_readahead=0x00020000
stubs num: 2
   INIT: 7.31
   flags=0x0040f039
   max_readahead=0x00020000
   max_write=0x00100000
   max_background=0
   congestion_threshold=0
   time_gran=1
   unique: 2, success, outsize: 80
unique: 4, opcode: LOOKUP (1), nodeid: 1, insize: 56, pid: 1146425
LOOKUP /hello-world.txt
getattr[NULL] /hello-world.txt
   NODEID: 2
   unique: 4, success, outsize: 144
unique: 6, opcode: OPEN (14), nodeid: 2, insize: 48, pid: 1146425
open flags: 0x8000 /hello-world.txt
0: hello-world.txt.0
1: hello-world.txt.1
Open: fi->fh:1
   open[1] flags: 0x8000 /hello-world.txt
   unique: 6, success, outsize: 32
unique: 8, opcode: LOOKUP (1), nodeid: 1, insize: 54, pid: 1146425
LOOKUP /dead-beef.txt
getattr[NULL] /dead-beef.txt
   NODEID: 3
   unique: 8, success, outsize: 144
unique: 10, opcode: OPEN (14), nodeid: 3, insize: 48, pid: 1146425
open flags: 0x8000 /dead-beef.txt
0: dead-beef.txt.0
1: dead-beef.txt.1
Open: fi->fh:2
   open[2] flags: 0x8000 /dead-beef.txt
   unique: 10, success, outsize: 32
unique: 12, opcode: LOOKUP (1), nodeid: 1, insize: 52, pid: 1146425
LOOKUP /abc-def.txt
getattr[NULL] /abc-def.txt
   NODEID: 4
   unique: 12, success, outsize: 144
unique: 14, opcode: OPEN (14), nodeid: 4, insize: 48, pid: 1146425
open flags: 0x8000 /abc-def.txt
0: abc-def.txt.0
1: abc-def.txt.1
Open: fi->fh:3
   open[3] flags: 0x8000 /abc-def.txt
   unique: 14, success, outsize: 32
unique: 16, opcode: READ (15), nodeid: 2, insize: 80, pid: 1146425
read[1] 16384 bytes from 0 flags: 0x8000
Read: fi->fh:1
nanafs_read: timeuout
this is timeout callbacl function
nanafs_read: timeuout
this is timeout callbacl function
   read[1] 0 bytes from 0
   unique: 16, success, outsize: 16
unique: 18, opcode: GETATTR (3), nodeid: 3, insize: 56, pid: 1146425
getattr[2] /dead-beef.txt
   unique: 18, success, outsize: 120
unique: 20, opcode: READ (15), nodeid: 3, insize: 80, pid: 1146425
read[2] 16384 bytes from 0 flags: 0x8000
Read: fi->fh:2
nanafs_read: timeuout
this is timeout callbacl function
nanafs_read: timeuout
this is timeout callbacl function
   read[2] 0 bytes from 0
   unique: 20, success, outsize: 16
unique: 22, opcode: GETATTR (3), nodeid: 4, insize: 56, pid: 1146425
getattr[3] /abc-def.txt
   unique: 22, success, outsize: 120
unique: 24, opcode: READ (15), nodeid: 4, insize: 80, pid: 1146425
read[3] 16384 bytes from 0 flags: 0x8000
Read: fi->fh:3
nanafs_read: timeuout
this is timeout callbacl function
nanafs_read: timeuout
this is timeout callbacl function
   read[3] 0 bytes from 0
   unique: 24, success, outsize: 16
unique: 26, opcode: FLUSH (25), nodeid: 2, insize: 64, pid: 1146425
   unique: 26, error: -38 (Function not implemented), outsize: 16
unique: 28, opcode: RELEASE (18), nodeid: 2, insize: 64, pid: 0
release[1] flags: 0x8000
   unique: 28, success, outsize: 16
unique: 30, opcode: RELEASE (18), nodeid: 3, insize: 64, pid: 0
unique: 32, opcode: RELEASE (18), nodeid: 4, insize: 64, pid: 0
release[3] flags: 0x8000
   unique: 32, success, outsize: 16
release[2] flags: 0x8000
   unique: 30, success, outsize: 16

```


# TODO
- multi threads of client
- specify server ip addr by client
