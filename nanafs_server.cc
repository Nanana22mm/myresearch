#define READDIR_FILE_SIZE 10000
#define FILENAME_LEN 100

#include <absl/strings/str_format.h>
#include <arpa/inet.h>
#include <ext/stdio_filebuf.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <malloc.h>
#include <memory>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "nanafs.grpc.pb.h"

using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using nanafs::CloseReply;
using nanafs::CloseRequest;
using nanafs::NanafsRPC;
using nanafs::OpenReply;
using nanafs::OpenRequest;
using nanafs::ReaddirReply;
using nanafs::ReaddirRequest;
using nanafs::ReadReply;
using nanafs::ReadRequest;

struct OpenFile
{
    char *addr; // mmap address
    long size;
};
unordered_map<int, OpenFile> open_files;

class NanafsService final : public NanafsRPC::Service
{
    Status Open(ServerContext *context, const OpenRequest *request,
                OpenReply *reply) override
    {
        int fd = open(request->pathname().c_str(), O_RDONLY);
        if (fd < 0)
        {
            return Status::CANCELLED;
        }
        reply->set_fd(fd);

        struct stat stat;
        if (fstat(fd, &stat) < 0)
        {
            return Status::CANCELLED;
        }

        char *addr = (char *)mmap(NULL, (size_t)stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (addr == MAP_FAILED)
        {
            return Status::CANCELLED;
        }

        open_files[fd] = OpenFile{addr, stat.st_size};

        cerr << "Open(" << request->pathname() << "): fd=" << fd << endl;
        return Status::OK;
    }

    Status
    Read(ServerContext *context, const ReadRequest *request,
         ReadReply *reply) override
    {
        int fd = request->fd();

        int count = request->count();
        int offset = request->offset();

        if (offset >= open_files[fd].size)
        {
            count = 0;
            offset = open_files[fd].size;
        }
        if (offset + count > open_files[fd].size)
        {
            count = open_files[fd].size - offset;
        }

        // NOTE: setup of timeout
        cout << "sleep for 10 sec" << endl;
        sleep(10);
        cout << "sleep finished" << endl;

        cerr << "Read(fd="
             << fd << ", count=" << request->count() << ", offset=" << request->offset()
             << "): len=" << count << ", size: " << open_files[fd].size << endl;

        char *readaddr = open_files[fd].addr;
        reply->set_len(count);
        reply->set_data(string(readaddr + offset, count));

        return Status::OK;
    }

    Status
    Close(ServerContext *context, const CloseRequest *request,
          CloseReply *reply) override
    {
        int fd = request->fd();
        if (close(fd) < 0)
        {
            reply->set_result(-1);
            cerr << "Close(fd="
                 << fd << "): failed" << endl;
            return Status::CANCELLED;
        }
        else
        {
            reply->set_result(0);
            cerr << "Close(fd="
                 << fd << "): success" << endl;
            munmap(open_files[fd].addr, open_files[fd].size);
            return Status::OK;
        }
    }

    Status Readdir(ServerContext *context, const ReaddirRequest *request,
                   ReaddirReply *reply) override
    {
        const char *filename = "files_list.txt";
        FILE *fp;
        if ((fp = fopen(filename, "r")) == NULL)
        {
            cerr << "Readdir: cannot open the file" << endl;
        }

        char *tmp_buf = (char *)calloc(READDIR_FILE_SIZE, sizeof(char));
        char *buf = (char *)calloc(FILENAME_LEN, sizeof(char));
        int filenum = 0;
        while (fscanf(fp, "%s", tmp_buf) != EOF)
        {
            printf("tmp_buf: %s\n", tmp_buf);
            strcat(buf, tmp_buf);
            strcat(buf, "\n");
            filenum++;
        }

        string dir_data(buf);
        dir_data[dir_data.length() - 1] = 0;
        reply->set_data(dir_data);
        reply->set_filenum(filenum);

        cerr << "Readdir(dirname="
             << request->dirname() << "): filenum=" << filenum << endl;
        return Status::OK;
    }
};

void RunServer(char *ipaddr, uint16_t port)
{
    string server_address = absl::StrFormat("0.0.0.0:%d", port);
    NanafsService service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;

    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);

    // Finally assemble the server.
    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "Server listening on " << server_address << endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        cerr << "Usage: ./nanafs_server <IP addr> <port>" << endl;
        return -1;
    }
    int port = atoi(argv[2]);

    RunServer(argv[1], port);
    return 0;
}
