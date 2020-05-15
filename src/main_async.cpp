#include <iostream>
#include <memory>
#include <string>
#include <opencv2/opencv.hpp>

#include <thread>
#include <grpcpp/grpcpp.h>

#include "rgbd.pb.h"
#include "rgbd.grpc.pb.h"

using grpc::Server;

using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;

using grpc::ServerCompletionQueue;
using grpc::Status;
using rgbd::RGBDRequest;
using rgbd::RGBDReply;
using rgbd::RGBDService;

class ServerImpl final {
 public:
  ~ServerImpl() {
    server_->Shutdown();
    cq_->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run() {
    std::string server_address("localhost:50051");

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    HandleRpcs();
  }


 private:
  // Class encompasing the state and logic needed to serve a request.
  class CallData {
   public:
    CallData(RGBDService::AsyncService* service, ServerCompletionQueue* cq)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
      // Invoke the serving logic right away.
      Proceed();
    }

    void Proceed() {
      cv::Mat rgb_img;      
      rgb_img = cv::imread("../image/lena.png"); 
      uint32_t width = rgb_img.cols;
      uint32_t height = rgb_img.rows;
      uint32_t channel = rgb_img.channels();
      /*
    	cv::Mat GaussNoise = cv::Mat::zeros(rgb_img.size(),CV_8UC3);
      randn(GaussNoise, 0 , 100);
      rgb_img += GaussNoise;
      */
      int image_size = rgb_img.total() * rgb_img.elemSize();
      char *image_char = new char[image_size];
      std::memcpy(image_char, rgb_img.data, image_size * sizeof(char));
      std::string data_rgb_str(reinterpret_cast<char const *>(image_char),
                            image_size);

      if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestRGBDSend(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);
      } else if (status_ == PROCESS) {
        new CallData(service_, cq_);
      
        rgbd::Image *rgb_image_pb = new rgbd::Image();
        rgb_image_pb->set_width(width);
        rgb_image_pb->set_height(height);
        rgb_image_pb->set_channel(channel);
        rgb_image_pb->set_data(data_rgb_str);
        reply_.set_allocated_rgb_image(rgb_image_pb);
        //reply_.mutable_rgb_image().MergeFrom(rgb_image_pb);
        //reply_.set_rgb_image(data_rgb_str);
        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
      } else {
        GPR_ASSERT(status_ == FINISH);
        delete this;
      }
    }  

   private:
    RGBDService::AsyncService* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;

    RGBDRequest request_;
    RGBDReply reply_;

    // The means to get back to the client.
    ServerAsyncResponseWriter<RGBDReply> responder_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
  };

  void HandleRpcs() {
    // Spawn a new CallData instance to serve new clients.
    new CallData(&service_, cq_.get());
    void* tag;  // uniquely identifies a request.
    bool ok;

    while (true) {
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<CallData*>(tag)->Proceed();
    }
  }  

  std::unique_ptr<ServerCompletionQueue> cq_;
  RGBDService::AsyncService service_;
  std::unique_ptr<Server> server_;
};

int main(int argc, char** argv) {
  ServerImpl server;
  server.Run();

  return 0;
}

