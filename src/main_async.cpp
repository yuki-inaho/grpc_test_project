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

  void Configure(){
  device_index_ = device_index__;
  usleep(5 * 1e6);  // To avoid high frequent sensor open call from immediately
                    // after termination and rebooting

  if (!checkFileExistence(cfgParamPath)) {
    std::cerr << "Setting TOML file does not exist" << std::endl;
    std::exit(EXIT_FAILURE);
  }
  ParameterManager cfgParam(cfgParamPath);

  sensor_id_ = cfgParam.readStringData("General", "sensor_id");

  if (!cfgParam.checkExistanceTable(camKey.c_str())) {
    std::cerr << "Camera name is invalid, please check setting toml file"
              << std::endl;
    std::exit(EXIT_FAILURE);
  }

  std::string camera_name =
      cfgParam.readStringData(camKey.c_str(), "camera_name");
  serial_no_ = cfgParam.readStringData(camKey.c_str(), "serial_no");
  range1 = cfgParam.readIntData(camKey.c_str(), "range1");
  range2 = cfgParam.readIntData(camKey.c_str(), "range2");
  isRGB = cfgParam.readIntData(camKey.c_str(), "rgb_image") == 1;
  isWDR = (range1 >= 0) && (range2 >= 0);

  // TODO: merge factory values and tuned values
  std::string camFactKey = camKey + "_Factory";

  CameraParameter camera_factory_param;
  camera_factory_param.fx = cfgParam.readFloatData(camFactKey.c_str(), "fx");
  camera_factory_param.fy = cfgParam.readFloatData(camFactKey.c_str(), "fy");
  camera_factory_param.cx = cfgParam.readFloatData(camFactKey.c_str(), "cx");
  camera_factory_param.cy = cfgParam.readFloatData(camFactKey.c_str(), "cy");
  camera_factory_param.p1 = cfgParam.readFloatData(camFactKey.c_str(), "p1");
  camera_factory_param.p2 = cfgParam.readFloatData(camFactKey.c_str(), "p2");
  camera_factory_param.k1 = cfgParam.readFloatData(camFactKey.c_str(), "k1");
  camera_factory_param.k2 = cfgParam.readFloatData(camFactKey.c_str(), "k2");
  camera_factory_param.k3 = cfgParam.readFloatData(camFactKey.c_str(), "k3");
  camera_factory_param.k4 = cfgParam.readFloatData(camFactKey.c_str(), "k4");
  camera_factory_param.k5 = cfgParam.readFloatData(camFactKey.c_str(), "k5");
  camera_factory_param.k6 = cfgParam.readFloatData(camFactKey.c_str(), "k6");

  std::string id_str;
  std::cout << "Serial number allocated to Zense Publisher : " << serial_no_
            << std::endl;

  /* Define Undistortion Module */
  std::string distortionKey = camKey + "_Undistortion";

  // If TOML configuration file doesn't contain any undistortion
  // description(table), undistortion process will be skip
  if (cfgParam.checkExistanceTable(distortionKey)) {
    undistortion_flag = cfgParam.readBoolData(distortionKey, "flag");
  } else {
    undistortion_flag = false;
  }
  if (undistortion_flag == true) {
    undistorter = PicoZenseUndistorter(cfgParam, distortionKey);
  }

  manager_.openDevice(device_index_);
  if (!manager_.setupDevice(device_index_, range1, range2, isRGB)) {
    close();
    std::cerr << "Could not setup device" << std::endl;
    std::exit(EXIT_FAILURE);
  }
  if (range2 < 0) range2 = range1;

  camera_param = manager_.getCameraParameter(device_index_, 0);
  if (!(isWithinError(camera_param.fx, camera_factory_param.fx) &&
        isWithinError(camera_param.fy, camera_factory_param.fy) &&
        isWithinError(camera_param.cx, camera_factory_param.cx) &&
        isWithinError(camera_param.cy, camera_factory_param.cy) &&
        isWithinError(camera_param.p1, camera_factory_param.p1) &&
        isWithinError(camera_param.p2, camera_factory_param.p2) &&
        isWithinError(camera_param.k1, camera_factory_param.k1) &&
        isWithinError(camera_param.k2, camera_factory_param.k2) &&
        isWithinError(camera_param.k3, camera_factory_param.k3) &&
        isWithinError(camera_param.k4, camera_factory_param.k4) &&
        isWithinError(camera_param.k5, camera_factory_param.k5) &&
        isWithinError(camera_param.k6, camera_factory_param.k6))) {
    close();
    std::cerr << "Erroneous internal parameters. Exiting..." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  if (!manager_.startDevice(device_index_)) {
    close();
    std::cerr << "Could not start device" << std::endl;
    std::exit(EXIT_FAILURE);
  }
  std::string ns = "/" + camera_name;

  std::cout << "Camera setup is finished!" << std::endl;

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

