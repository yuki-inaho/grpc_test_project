#! /usr/bin/python

import time
import grpc
import rgbd_pb2
import rgbd_pb2_grpc
from utils import ndarray_to_bytes
from concurrent import futures
from zense_pywrapper import PyPicoZenseManager
import cv2
import numpy as np
import pdb

class RGBDServiceServicer(rgbd_pb2_grpc.RGBDServiceServicer):
    def __init__(self):
        self.zense_mng = PyPicoZenseManager(0)

    def RGBDSend(self, request, context):
        while not self.zense_mng.update():
            pass
        rgb_img = self.zense_mng.getRGBImage()
        depth_img = self.zense_mng.getDepthImage()
        rgb_img_pb = ndarray_to_bytes(rgb_img)
        depth_img_pb = ndarray_to_bytes(depth_img)
        return rgbd_pb2.RGBDReply(rgb_image=rgb_img_pb,
                                  depth_image=depth_img_pb)


server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))
rgbd_pb2_grpc.add_RGBDServiceServicer_to_server(RGBDServiceServicer(), server)
server.add_insecure_port('localhost:50051')
server.start()
print('start')

try:
    while True:
        time.sleep(100)
except KeyboardInterrupt:
    server.stop(0)