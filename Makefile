PROTO_NAME=nanafs
METASRV_PROTO_NAME=nanafs_metasrv

PB_HEADERS = $(PROTO_NAME).grpc.pb.h $(PROTO_NAME).pb.h
PB_CC_SRCS = $(PROTO_NAME).grpc.pb.cc $(PROTO_NAME).pb.cc
PB_OBJS = $(PROTO_NAME).grpc.pb.o $(PROTO_NAME).pb.o

METASRV_PB_HEADERS = $(METASRV_PROTO_NAME).grpc.pb.h $(METASRV_PROTO_NAME).pb.h
METASRV_PB_CC_SRCS = $(METASRV_PROTO_NAME).grpc.pb.cc $(METASRV_PROTO_NAME).pb.cc
METASRV_PB_OBJS = $(METASRV_PROTO_NAME).grpc.pb.o $(METASRV_PROTO_NAME).pb.o

GRPC_INSTALL_DIR=/home/nana/b4research_saito/grpc-install

LDFLAGS = $(shell pkg-config --libs --static grpc++ fuse3 protobuf) -L$(GRPC_INSTALL_DIR)/lib -lgrpc++_reflection -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_calib3d -lopencv_dnn -lopencv_features2d -lopencv_flann -lopencv_gapi -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_videoio -lopencv_video
CXXFLAGS = $(shell pkg-config --libs --cflags grpc++ fuse3 protobuf) -std=c++17 -I$(GRPC_INSTALL_DIR)/include  -I/usr/include/opencv4/ -g
CFLAGS += -D_FILE_OFFSET_BITS=64

all: nanafs_client nanafs_server nanafs_metasrv

nanafs_client: nanafs.o $(PB_OBJS) $(PB_HEADERS) $(METASRV_PB_OBJS) $(METASRV_PB_HEADERS)
	$(CXX) $< $(PB_OBJS) $(METASRV_PB_OBJS) -o $@ $(LDFLAGS)

nanafs_server: nanafs_server.o $(PB_OBJS) $(PB_HEADERS)
	$(CXX) $< $(PB_OBJS) -o $@ $(LDFLAGS)

nanafs_metasrv: nanafs_metasrv.o $(METASRV_PB_OBJS) $(METASRV_PB_HEADERS)
	$(CXX) $< $(METASRV_PB_OBJS) -o $@ $(LDFLAGS)

nanafs_server.o: nanafs_server.cc $(PB_HEADERS)
	$(CXX) $(CXXFLAGS) -c $<

nanafs_metasrv.o: nanafs_metasrv.cc $(METASRV_PB_HEADERS)
	$(CXX) $(CXXFLAGS) -c $<

nanafs.o: nanafs.cc $(PB_HEADERS) $(METASRV_PB_HEADERS)
	$(CXX) $(CXXFLAGS) -c $<


$(PROTO_NAME).grpc.pb.h $(PROTO_NAME).grpc.pb.cc: $(PROTO_NAME).proto
	protoc --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $(PROTO_NAME).proto

$(PROTO_NAME).pb.h $(PROTO_NAME).pb.cc: $(PROTO_NAME).proto
	protoc --cpp_out=. $(PROTO_NAME).proto

$(METASRV_PROTO_NAME).grpc.pb.h $(METASRV_PROTO_NAME).grpc.pb.cc: $(METASRV_PROTO_NAME).proto
	protoc --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $(METASRV_PROTO_NAME).proto

$(METASRV_PROTO_NAME).pb.h $(METASRV_PROTO_NAME).pb.cc: $(METASRV_PROTO_NAME).proto
	protoc --cpp_out=. $(METASRV_PROTO_NAME).proto

clean:
	rm -f nanafs_client nanafs_server nanafs_metasrv *.o *.pb.cc *.pb.h
