#define FUSE_USE_VERSION 34
#define FILENAME_LEN 100
#define MY_TIMEOUT 1000 // msec

#define SERVER_STATE_READY 0
#define SERVER_STATE_NOT_READY 1

#include <fuse.h>
#include <fuse_lowlevel.h>
#include <fstream>
#include <iostream>
#include <malloc.h>
#include <sys/mman.h>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <grpcpp/grpcpp.h>
#include "nanafs.grpc.pb.h"
#include "nanafs_metasrv.grpc.pb.h"
#include "measure_myfuse.h"

using namespace std;
using grpc::Channel;
using grpc::ClientContext;
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

using nanafs_metaserver::MountReply;
using nanafs_metaserver::MountRequest;
using nanafs_metaserver::NanafsMetaServerRPC;

vector<nanafs::NanafsRPC::Stub> stubs;
vector<char *> separate_reply_msg(char *msg, int num, char separater);

static void *nanafs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    (void)conn;
    cfg->kernel_cache = 1;

    // connect with metasrv
    // for setting ip addr
    const string metasrv_ip = "127.0.0.1";
    const string metasrv_port = "50070";
    const string metasrv_channel = metasrv_ip + ":" + metasrv_port;
    auto channel_metasrv = grpc::CreateChannel(
        metasrv_channel, grpc::InsecureChannelCredentials());

    MountReply reply;
    MountRequest request;
    ClientContext context;
    Status status = NanafsMetaServerRPC::Stub(channel_metasrv).Mount(&context, request, &reply);

    if (!status.ok())
    {
        cerr << "nanafs_mount:" << status.error_details() << endl;
        return NULL;
    }

    vector<char *> ipaddr_list = separate_reply_msg((char *)reply.ipaddr().c_str(), reply.server_num(), '\n');
    vector<char *> port_list = separate_reply_msg((char *)reply.port().c_str(), reply.server_num(), '\n');

    static string hosts[] = {
        "0.0.0.0:50059",
        "0.0.0.0:50056",

        // NOTE: need to comment-out below three line when using config_file_two.txt
        //  "0.0.0.0:50057",
        //  "0.0.0.0:50058",
        //  "0.0.0.0:50060",
    };

    // NOTE: comment-out below (maybe do not move using this for loop)
    // for setting ip addr
    // static vector<string> hosts;
    // for (int i = 0; i < ipaddr_list.size(); i++)
    // {
    //     const string server_channel = string(ipaddr_list[i]) + ":" + string(port_list[i]);
    //     hosts.push_back(server_channel);
    //     cout << i << ";"
    //          << "ip: " << ipaddr_list[i] << ", port: " << port_list[i] << ", server_channel: " << server_channel << endl;
    // }

    for (int i = 0; i < reply.server_num(); i++)
    {
        auto channel = grpc::CreateChannel(
            hosts[i], grpc::InsecureChannelCredentials());
        stubs.emplace_back(NanafsRPC::Stub(channel));
    }

    cout << "stubs num: " << stubs.size() << endl;
    return NULL;
}

struct NanafsFilePart
{
    int server_state = SERVER_STATE_READY;
    int fd;
    nanafs::NanafsRPC::Stub *stub;
};

struct NanafsFile
{
    vector<NanafsFilePart> parts;
};
unordered_map<int, NanafsFile *> open_files;
int file_num;

vector<u_char> blue_array;
vector<u_char> green_array;
vector<u_char> red_array;

void timeout()
{
    cout << "this is timeout callbacl function\n";

    // TODO: define the timeout callback function
    return;
}

static int
nanafs_open(const char *path, struct fuse_file_info *fi)
{
    NanafsFile *f = new NanafsFile();

    for (int i = 0; i < stubs.size(); i++)
    {
        stringstream part_path;
        part_path << path + 1;
        // part_path << ".bin." << i; //*.bmp.bin.0, *.bmp.bin.1
        part_path << "." << i; // *.txt.0, *.txt.1

        OpenRequest request;
        OpenReply reply;
        ClientContext context;

        cerr << i << ": " << part_path.str() << endl;
        request.set_pathname(part_path.str());
        auto stub = stubs[i];

        Status status = stub.Open(&context, request, &reply);

        if (!status.ok())
        {
            cerr << "nanafs_open: " << status.error_details() << endl;
            return -1;
        }

        int fd = reply.fd();
        if (fd < 0)
        {
            cerr << "nanafs_open: failed to get fd" << endl;
            return -1;
        }
        NanafsFilePart *p = new NanafsFilePart();
        p->fd = fd;
        p->server_state = SERVER_STATE_READY;
        p->stub = &stubs[i];
        f->parts.push_back(*p);
    }

    file_num++;
    fi->fh = file_num;
    open_files[file_num] = f;
    cout << "Open: fi->fh:" << fi->fh << endl;
    return 0;
}

static int
nanafs_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi)
{
    uint64_t open_time = 0;
    uint64_t close_time = 0;
    uint64_t read_start = 0;
    uint64_t read_end = 0;
    cout << "Read: fi->fh:" << fi->fh << endl;
    NanafsFile *f = open_files[fi->fh];
    int len = 0;
    string s;

    for (int i = 0; i < stubs.size(); i++)
    {
        ReadRequest request;
        ReadReply reply;
        ClientContext context;

        int fd = f->parts[i].fd;
        request.set_fd(fd);
        request.set_count((int)size / 2);
        request.set_offset((int)offset);

        chrono::system_clock::time_point deadline = chrono::system_clock::now() + chrono::milliseconds(MY_TIMEOUT);
        context.set_deadline(deadline); // NOTE: setup of timeout

        Status status = f->parts[i].stub->Read(&context, request, &reply);

        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
        {
            cerr << "nanafs_read: timeuout" << endl;
            f->parts[i].server_state = SERVER_STATE_NOT_READY;
            timeout();
        }
        else if (!status.ok())
        {
            cerr << "nanafs_read: " << status.error_details() << endl;
            return -1;
        }
        else
        {
            // TODO: devide color array here

            s += reply.data();
            if (reply.len() > 0)
            {
                len += reply.len(); // reply.len() is 1/2 of size(argument) == len(return)
            }
            cout << i << ": " << s << endl;
        }
    }
    strcpy(buf, s.c_str());
    return len;
}

vector<char *> separate_reply_msg(char *msg, int num, char separater)
{
    // tokenize string of file list
    int count = 0;
    auto list = vector<char *>();

    char *token = (char *)calloc(FILENAME_LEN, sizeof(char));
    token = strtok(msg, &separater);
    // printf("token: %s\n", token);
    count++;
    char *str = (char *)calloc(FILENAME_LEN, sizeof(char));
    strcpy(str, token);
    list.push_back(str);

    while (token != NULL || count < num)
    {
        token = strtok(NULL, &separater);
        // printf("token: %s\n", token);
        count++;
        char *str = (char *)calloc(FILENAME_LEN, sizeof(char));
        strcpy(str, token);
        list.push_back(str);
        if (count >= num)
        {
            break;
        }
    }
    return list;
}

static int
nanafs_close(const char *path, struct fuse_file_info *fi)
{
    NanafsFile *f = open_files[fi->fh];
    for (int i = 0; i < stubs.size(); i++)
    {
        if (f->parts[i].server_state == SERVER_STATE_NOT_READY)
        {
            continue;
        }

        CloseRequest request;
        CloseReply reply;
        ClientContext context;

        int fd = f->parts[i].fd;
        request.set_fd(fd);

        chrono::system_clock::time_point deadline = chrono::system_clock::now() + chrono::milliseconds(MY_TIMEOUT);
        // context.set_deadline(deadline); // NOTE: setup of timeout

        Status status = f->parts[i].stub->Close(&context, request, &reply);

        if (!status.ok())
        {
            cerr << "nanafs_close: " << status.error_details() << endl;
            return -1;
        }
        if (reply.result() < 0)
        {
            cerr << "nanaf_close: close failed" << endl;
            return -1;
        }
    }

    free(&open_files[fi->fh]->parts);
    open_files.erase(fi->fh);

    return 0;
}

static int
nanafs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info *fi,
               enum fuse_readdir_flags flags)
{
    // memo: path == "/" ordinarily
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    ReaddirRequest request;
    ReaddirReply reply;
    ClientContext context;

    request.set_dirname("tmp_dirname");
    Status status = stubs[0].Readdir(&context, request, &reply);
    if (!status.ok())
    {
        cerr << "nanafs_readdir: " << status.error_details() << endl;
        return -1;
    }
    string file_list = reply.data();
    int file_num = reply.filenum();

    // tokenize string of file list
    char *msg = (char *)file_list.c_str();
    vector<char *> list = separate_reply_msg(msg, file_num, '\n');

    filler(buf, ".", NULL, 0, (enum fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (enum fuse_fill_dir_flags)0);
    for (int i = 0; i < file_num; i++)
    {
        filler(buf, list[i], NULL, 0, (enum fuse_fill_dir_flags)0);
    }
    return 0;
}

static int nanafs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    (void)fi;
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755; // rwxr-xr-x
        stbuf->st_nlink = 2;
    }
    else
    {
        stbuf->st_mode = S_IFREG | 0444; // r--r--r--
        stbuf->st_nlink = 1;
        stbuf->st_size = 0xdeadbeef;
    }
    return res;
}

const struct fuse_operations nanafs_ops = {
    .getattr = nanafs_getattr, // this function used before open file
    .open = nanafs_open,
    .read = nanafs_read,
    .release = nanafs_close,
    .readdir = nanafs_readdir,
    .init = nanafs_init,
};

int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    int ret = fuse_main(args.argc, args.argv, &nanafs_ops, NULL);

    fuse_opt_free_args(&args);
    return ret;
}
