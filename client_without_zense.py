#! /usr/bin/python
import grpc
import rgbd_pb2
import rgbd_pb2_grpc
from utils import bytes_to_ndarray
import sys
import pdb

while True:
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = rgbd_pb2_grpc.RGBDServiceStub(channel)
        response = stub.RGBDSend(rgbd_pb2.RGBDRequest())
        print("ok")
        #depth_image = bytes_to_ndarray(response.depth_image)
