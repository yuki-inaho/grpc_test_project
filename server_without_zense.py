#! /usr/bin/python

import time
import grpc
import rgbd_pb2
import rgbd_pb2 as Image
import rgbd_pb2_grpc
from utils import ndarray_to_bytes
from concurrent import futures
import cv2

import pdb
class RGBDServiceServicer(rgbd_pb2_grpc.RGBDServiceServicer):
    def __init__(self):
        pass

    def RGBDSend(self, request, context):
        rgb_img = cv2.imread("./image/rgb.png")
        depth_img = cv2.imread("./image/depth.png", cv2.IMREAD_ANYDEPTH)
        rgb_img_pb = ndarray_to_bytes(rgb_img)
        depth_img_pb = ndarray_to_bytes(depth_img)
        rgb_img_pb = rgbd_pb2.Image(data=ndarray_to_bytes(rgb_img))
        depth_img_pb = rgbd_pb2.Image(data=ndarray_to_bytes(depth_img))
        return rgbd_pb2.RGBDReply(rgb_image=rgb_img_pb, depth_image=depth_img_pb)


server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))
rgbd_pb2_grpc.add_RGBDServiceServicer_to_server(RGBDServiceServicer(), server)
server.add_insecure_port('localhost:50051')
server.start()
print('start')

try:
    while True:
        time.sleep(1000)
except KeyboardInterrupt:
    server.stop(0)