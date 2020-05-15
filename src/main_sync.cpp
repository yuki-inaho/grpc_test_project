#include <iostream>
#include <memory>
#include <string>
#include <opencv2/opencv.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "rgbd.pb.h"
#include "rgbd.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using rgbd::RGBDRequest;
using rgbd::RGBDReply;
using rgbd::RGBDService;

// Logic and data behind the server's behavior.
class RGBDServiceImpl final : public rgbd::RGBDService::Service {
  Status RGBDSend(ServerContext* context, const RGBDRequest* request,
                  RGBDReply* reply) override {
    cv::Mat rgb_img = cv::imread("../image/rgb.png");    
    int width = rgb_img.cols;
    int height = rgb_img.rows;
    int channel = rgb_img.channels();
    int image_size = rgb_img.total() * rgb_img.elemSize();
    char *image_char = new char[image_size];
    std::memcpy(image_char, rgb_img.data, image_size * sizeof(char));
    std::string data_rgb_str(reinterpret_cast<char const *>(image_char),
                            image_size);
    //reply->mutable_rgb_image().Merge_From;
    reply->set_rgb_image(data_rgb_str);
    rgb_img.release();    

    //reply->set_allocated_rgb_image(&data_rgb_str);
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("localhost:50051");
  RGBDServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}

