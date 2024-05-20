#include <stdio.h>
#include <stdlib.h>
#include <absl/strings/str_format.h>
#include <arpa/inet.h>
#include <ext/stdio_filebuf.h>
#include <fstream>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "nanafs_metasrv.grpc.pb.h"

using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using nanafs_metaserver::MountReply;
using nanafs_metaserver::MountRequest;
using nanafs_metaserver::NanafsMetaServerRPC;

#define DIGIT 2 // digit of server num
#define LEN 20  // ipaddr len

class NanafsMetaServer final : public NanafsMetaServerRPC::Service
{
    Status Mount(ServerContext *context, const MountRequest *request,
                 MountReply *reply) override
    {
        // const char *filename = "config_file.txt";
        const char *filename = "config_file_two.txt";
        FILE *fp;
        if ((fp = fopen(filename, "r")) == NULL)
        {
            cerr << "Mount: cannot open the file" << endl;
            return Status::CANCELLED;
        }

        char *num = (char *)calloc(DIGIT, sizeof(char));
        int server_num = atoi(fgets(num, DIGIT, fp));
        char *tmp_buf = (char *)calloc(LEN, sizeof(char));
        char *ipaddr_buf = (char *)calloc(server_num * LEN, sizeof(char));
        char *port_buf = (char *)calloc(server_num * LEN, sizeof(char));
        int filenum = 0;
        while (fscanf(fp, "%s", tmp_buf) != EOF)
        {
            // printf("tmp_buf: %s\n", tmp_buf);
            strcat(ipaddr_buf, tmp_buf);
            strcat(ipaddr_buf, "\n");

            fscanf(fp, "%s", tmp_buf);
            strcat(port_buf, tmp_buf);
            strcat(port_buf, "\n");
        }
        reply->set_server_num(server_num);
        reply->set_ipaddr(ipaddr_buf);
        reply->set_port(port_buf);

        cout << "ip list:" << ipaddr_buf << endl;
        cout << "port list:" << port_buf << endl;
        cerr << "server_num: " << server_num << endl;
        return Status::OK;
    }
};

void RunServer(uint16_t port)
{
    // string server_address = absl::StrFormat("0.0.0.0:%d", port);
    string server_address = absl::StrFormat("127.0.0.1:%d", port);
    NanafsMetaServer service;

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
    cout << "Meta Server listening on " << server_address << endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        cerr << "Usage: ./nanafs_metasrv <port>" << endl;
        return -1;
    }
    int port = atoi(argv[1]);
    RunServer(port);
    return 0;
}
