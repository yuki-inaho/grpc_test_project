#! /usr/bin/python
import grpc
import rgbd_pb2
import rgbd_pb2_grpc
from utils import bytes_to_ndarray
import sys

with grpc.insecure_channel('localhost:50051') as channel:
    stub = rgbd_pb2_grpc.RGBDServiceStub(channel)
    response = stub.RGBDSend(rgbd_pb2.RGBDRequest())
    rgb_image = bytes_to_ndarray(response.rgb_image)
    depth_image = bytes_to_ndarray(response.depth_image)
    
