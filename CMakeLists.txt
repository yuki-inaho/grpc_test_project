cmake_minimum_required(VERSION 3.5.1)

project(grpc_test_project)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


#OpenCV
find_package(OpenCV 3.4.3 REQUIRED)
find_package(PkgConfig REQUIRED)

#Protobuf
find_package(Protobuf CONFIG REQUIRED)
set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

get_filename_component(hw_proto_rgbd "${CMAKE_CURRENT_BINARY_DIR}/../protos/rgbd.proto" ABSOLUTE)
get_filename_component(hw_proto_rgbd_path "${hw_proto_rgbd}" PATH)

set(hw_proto_rgbd_srcs "${CMAKE_CURRENT_BINARY_DIR}/../protos/rgbd.pb.cc")
set(hw_proto_rgbd_hdrs "${CMAKE_CURRENT_BINARY_DIR}/../protos/rgbd.pb.h")
set(hw_grpc_rgbd_srcs "${CMAKE_CURRENT_BINARY_DIR}/../protos/rgbd.grpc.pb.cc")
set(hw_grpc_rgbd_hdrs "${CMAKE_CURRENT_BINARY_DIR}/../protos/rgbd.grpc.pb.h")
add_custom_command(
      OUTPUT "${hw_proto_rgbd_srcs}" "${hw_proto_rgbd_hdrs}" "${hw_grpc_rgbd_srcs}" "${hw_grpc_rgbd_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}/../protos"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}/../protos"
        -I "${hw_proto_rgbd_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto_rgbd}"
      DEPENDS "${hw_proto_rgbd}")

# Include generated *.pb.h files
include_directories(
      protos
      "${CMAKE_CURRENT_BINARY_DIR}"
    )

set(SOURCE_CODE
  src/main_async.cpp
)


foreach(_target main_async)
  add_executable(${_target} "src/${_target}.cpp"
    ${hw_proto_rgbd_srcs}
    ${hw_grpc_rgbd_srcs}
    ${SOURCE_CODE}
    )
  target_link_libraries(${_target}
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    ${OpenCV_LIBRARIES}
    )
endforeach()